#include "arq.hpp"

bool gbn_sender(int fd, const std::vector<Frame> &frames,
                const GBNConfig &cfg) {
	size_t base{};
	size_t nextSeq{};

	while (base < frames.size()) {
		while (nextSeq < frames.size() && nextSeq < base + cfg.window_size) {
			std::vector<uint8_t> raw= serialize(frames[nextSeq]);
			send_bytes(fd, raw);
			nextSeq++;
	}
		std::vector<uint8_t> raw;

		bool ok = recv_bytes(
			fd,
			raw,
			11,
			cfg.timeout_ms
		);

		if (!ok) {
			for (size_t i = base; i < nextSeq; i++) {
        auto raw = serialize(frames[i]);
        send_bytes(fd, raw);
			}
			continue;
		}
		Frame ack = deserialize(raw);
		if (ack.type == FrameType::ACK)
			if (ack.seq >= base)
			base = ack.seq + 1;
	}
	if (base == frames.size())
		return true;
	else 
		return false;
}
