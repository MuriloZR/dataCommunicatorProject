#include "arq.hpp"
#include "socket.hpp"
#include <chrono>
#include <random>

RecvStatus receive_frame(int fd, std::vector<uint8_t>& raw, int timeout_ms) {
    auto frame_start = std::chrono::steady_clock::now();

    std::vector<uint8_t> header;
	RecvStatus header_status = recv_bytes(fd, header, 8, timeout_ms);
	if (header_status != RecvStatus::Ok) {
		return header_status;
    }

    uint16_t payload_size = (static_cast<uint16_t>(header[6]) << 8) | header[7];
    std::vector<uint8_t> tail;
    int remaining_timeout_ms = timeout_ms;

    if (timeout_ms > 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - frame_start
        ).count();

        remaining_timeout_ms = timeout_ms - static_cast<int>(elapsed);

        if (remaining_timeout_ms <= 0) {
			return RecvStatus::Timeout;
        }
    }

	RecvStatus tail_status = recv_bytes(fd, tail, static_cast<size_t>(payload_size) + 3, remaining_timeout_ms);
	if (tail_status != RecvStatus::Ok) {
		return tail_status;
    }

    raw = std::move(header);
    raw.insert(raw.end(), tail.begin(), tail.end());
	return RecvStatus::Ok;
}

Frame make_ack(const Frame& frame, uint16_t seq) {
    Frame ack{};
    ack.flag_start = 0x7E;
    ack.src = frame.dst;
    ack.dst = frame.src;
    ack.type = FrameType::ACK;
    ack.seq = seq;
    ack.length = 0;
    ack.flag_end = 0x7E;
    return ack;
}

bool should_drop(float loss_prob, std::mt19937& rng) {
    if (loss_prob <= 0.0f)
        return false;
    if (loss_prob >= 1.0f)
        return true;

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng) < loss_prob;
}

bool gbn_sender(int fd, const std::vector<Frame> &frames,
                const GBNConfig &cfg) {
	if (cfg.window_size == 0) {
		return frames.empty();
	}

	size_t base{};
	size_t nextSeq{};

	while (base < frames.size()) {
		while (nextSeq < frames.size() && nextSeq < base + cfg.window_size) {
			std::vector<uint8_t> raw = serialize(frames[nextSeq]);
			if (!send_bytes(fd, raw))
				return false;
			nextSeq++;
		}
		std::vector<uint8_t> raw;

		RecvStatus status = recv_bytes(
			fd,
			raw,
			11,
			cfg.timeout_ms
		);

		if (status == RecvStatus::Timeout) {
			for (size_t i = base; i < nextSeq; i++) {
				auto raw = serialize(frames[i]);
				if (!send_bytes(fd, raw)) {
					return false;
				}
			}
			continue;
		}
		if (status != RecvStatus::Ok) {
			return false;
		}

		auto ack = deserialize(raw);
		if (!ack) {
			continue;
		}
		if (ack->type == FrameType::ACK && ack->seq >= base) {
			base = static_cast<size_t>(ack->seq) + 1;
		}
	}

	return base == frames.size();
}

void gbn_receiver(int fd, const GBNConfig& cfg, std::vector<Frame>& out) {
	std::mt19937 rng{std::random_device{}()};
	uint16_t expected_seq{};
	bool have_ack{};
	uint16_t last_ack_seq{};

	while (true) {
		std::vector<uint8_t> raw;
		RecvStatus status = receive_frame(fd, raw, cfg.timeout_ms);
		if (status == RecvStatus::Timeout) {
			continue;
		}
		if (status != RecvStatus::Ok) {
			return;
		}

		auto frame = deserialize(raw);
		if (!frame) {
			continue;
		}
		bool is_termination = frame->type == FrameType::FIN ||
			(frame->type == FrameType::DATA && frame->payload.empty());

		if (frame->type == FrameType::ACK || frame->type == FrameType::NAK) {
			continue;
		}

		if (should_drop(cfg.loss_prob, rng)) {
			continue;
		}

		if (frame->seq == expected_seq) {
			if (!is_termination) {
				out.push_back(*frame);
			}
			last_ack_seq = expected_seq;
			have_ack = true;
			expected_seq++;
			if (is_termination) {
				Frame ack = make_ack(*frame, last_ack_seq);
				if (!send_bytes(fd, serialize(ack))) {
					return;
				}
				return;
			}
		}
		else if (frame->seq < expected_seq && expected_seq > 0) {
			last_ack_seq = static_cast<uint16_t>(expected_seq - 1);
			have_ack = true;
		}

		if (have_ack) {
			Frame ack = make_ack(*frame, last_ack_seq);
			if (!send_bytes(fd, serialize(ack))) {
				return;
			}
		}
	}
}
