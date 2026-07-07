#include "parse.hpp"
#include <charconv>
#include <fstream>

bool parse_int(const std::string& text, int& value) {
	auto begin = text.data();
	auto end = text.data() + text.size();
	auto result = std::from_chars(begin, end, value);
	return result.ec == std::errc{} && result.ptr == end;
}

bool parse_uint8(const std::string& text, uint8_t& value) {
	int parsed{};
	if (!parse_int(text, parsed) || parsed < 0 || parsed > 127)
		return false;
	value = static_cast<uint8_t>(parsed);
	return true;
}

bool parse_float(const std::string& text, float& value) {
	try {
		value = std::stof(text);
		return true;
	} catch (...) {
		return false;
	}
}

bool parse_server_args(int argc, char** argv, Options& options) {
	for (int i = 2; i < argc; ++i) {
		std::string arg = argv[i];

		if (arg == "--port" && i + 1 < argc) {
			if (!parse_int(argv[++i], options.port))
				return false;
		} else if (arg == "--loss" && i + 1 < argc) {
			if (!parse_float(argv[++i], options.loss_prob))
				return false;
		} else if (arg == "--timeout" && i + 1 < argc) {
			if (!parse_int(argv[++i], options.timeout_ms))
				return false;
		} else if (arg == "--window" && i + 1 < argc) {
			if (!parse_uint8(argv[++i], options.window_size))
				return false;
		} else if (arg == "--output" && i + 1 < argc) {
			options.output_file = argv[++i];
		} else {
			return false;
		}
	}

	return options.port > 0 && options.timeout_ms >= 0;
}

bool parse_client_args(int argc, char** argv, Options& options) {
	for (int i = 2; i < argc; ++i) {
		std::string arg = argv[i];

		if (arg == "--host" && i + 1 < argc) {
			options.host = argv[++i];
		} else if (arg == "--port" && i + 1 < argc) {
			if (!parse_int(argv[++i], options.port))
				return false;
		} else if (arg == "--file" && i + 1 < argc) {
			options.input_file = argv[++i];
		} else if (arg == "--timeout" && i + 1 < argc) {
			if (!parse_int(argv[++i], options.timeout_ms))
				return false;
		} else if (arg == "--window" && i + 1 < argc) {
			if (!parse_uint8(argv[++i], options.window_size))
				return false;
		} else {
			return false;
		}
	}

	return !options.host.empty() && !options.input_file.empty() && options.port > 0 && options.timeout_ms >= 0;
}

bool read_file_bytes(const std::string& path, std::vector<uint8_t>& data) {
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return false;

	file.seekg(0, std::ios::end);
	auto size = file.tellg();
	if (size < 0)
		return false;

	data.resize(static_cast<size_t>(size));
	file.seekg(0, std::ios::beg);
	if (!data.empty())
		file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));

	return file.good() || file.eof();
}

bool write_file_bytes(const std::string& path, const std::vector<uint8_t>& data) {
	std::ofstream file(path, std::ios::binary);
	if (!file)
		return false;

	if (!data.empty())
		file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

	return file.good();
}

std::string encode_hex(const std::vector<uint8_t>& input) {
	static constexpr char digits[] = "0123456789abcdef";
	std::string encoded;
	encoded.reserve(input.size() * 2);

	for (uint8_t byte : input) {
		encoded.push_back(digits[byte >> 4]);
		encoded.push_back(digits[byte & 0x0F]);
	}

	return encoded;
}

bool decode_hex(const std::vector<uint8_t>& encoded, std::vector<uint8_t>& output) {
	if (encoded.size() % 2 != 0)
		return false;

	auto hex_value = [](uint8_t c) -> int {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
		if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
		return -1;
	};

	output.clear();
	output.reserve(encoded.size() / 2);

	for (size_t i = 0; i < encoded.size(); i += 2) {
		int hi = hex_value(encoded[i]);
		int lo = hex_value(encoded[i + 1]);
		if (hi < 0 || lo < 0)
			return false;
		output.push_back(static_cast<uint8_t>((hi << 4) | lo));
	}

	return true;
}

Frame make_data_frame(uint16_t seq, std::vector<uint8_t> payload) {
	Frame frame{};
	frame.flag_start = 0x7E;
	frame.src = kClientAddress;
	frame.dst = kServerAddress;
	frame.type = FrameType::DATA;
	frame.seq = seq;
	frame.length = static_cast<uint16_t>(payload.size());
	frame.payload = std::move(payload);
	frame.flag_end = 0x7E;
	return frame;
}

std::vector<Frame> build_frames_from_file(const std::string& path) {
	std::vector<uint8_t> raw;
	if (!read_file_bytes(path, raw))
		return {};

	std::string encoded = encode_hex(raw);
	std::vector<Frame> frames;
	uint16_t seq{};

	for (size_t offset = 0; offset < encoded.size(); offset += kPayloadChunkSize) {
		auto chunk_end = std::min(encoded.size(), offset + kPayloadChunkSize);
		std::vector<uint8_t> payload(encoded.begin() + static_cast<std::ptrdiff_t>(offset),
		                             encoded.begin() + static_cast<std::ptrdiff_t>(chunk_end));
		frames.push_back(make_data_frame(seq++, std::move(payload)));
	}

	frames.push_back(make_data_frame(seq, {}));
	return frames;
}