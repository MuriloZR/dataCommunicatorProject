#include "arq.hpp"

#include <cctype>
#include <charconv>
#include <fstream>
#include <iostream>
#include <string>

namespace {

constexpr uint8_t kClientAddress = 1;
constexpr uint8_t kServerAddress = 2;
constexpr size_t kPayloadChunkSize = 128;

struct Options {
	std::string mode;
	std::string host{"127.0.0.1"};
	std::string input_file;
	std::string output_file{"received.bin"};
	int port{5000};
	uint8_t window_size{4};
	int timeout_ms{3000};
	float loss_prob{0.0f};
};

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

void print_usage(const char* program) {
	std::cerr
		<< "Uso:\n"
		<< "  " << program << " server [--port N] [--loss P] [--timeout MS] [--window N] [--output FILE]\n"
		<< "  " << program << " client --host IP --port N --file FILE [--timeout MS] [--window N]\n";
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

int run_server(const Options& options) {
	GBNConfig cfg{options.window_size, options.timeout_ms, options.loss_prob};
	int fd = create_server(options.port);

	std::vector<Frame> received_frames;
	gbn_receiver(fd, cfg, received_frames);
	close_socket(fd);

	std::vector<uint8_t> encoded_bytes;
	for (const auto& frame : received_frames)
		encoded_bytes.insert(encoded_bytes.end(), frame.payload.begin(), frame.payload.end());

	std::vector<uint8_t> decoded_bytes;
	if (!decode_hex(encoded_bytes, decoded_bytes)) {
		std::cerr << "Erro: payload recebido invalido para decodificacao hex.\n";
		return 1;
	}

	if (!write_file_bytes(options.output_file, decoded_bytes)) {
		std::cerr << "Erro: falha ao gravar arquivo de saida: " << options.output_file << '\n';
		return 1;
	}

	std::cout << "Servidor recebeu " << decoded_bytes.size()
	          << " bytes e salvou em " << options.output_file << '\n';
	return 0;
}

int run_client(const Options& options) {
	GBNConfig cfg{options.window_size, options.timeout_ms, 0.0f};
	auto frames = build_frames_from_file(options.input_file);
	if (frames.empty()) {
		std::cerr << "Erro: nao foi possivel ler o arquivo ou ele esta vazio: "
		          << options.input_file << '\n';
		return 1;
	}

	int fd = create_client(options.host.c_str(), options.port);
	bool ok = gbn_sender(fd, frames, cfg);
	close_socket(fd);

	if (!ok) {
		std::cerr << "Erro: o envio nao foi confirmado pelo servidor.\n";
		return 1;
	}

	std::cout << "Cliente enviou " << options.input_file << " com sucesso.\n";
	return 0;
}

} // namespace

int main(int argc, char** argv) {
	if (argc < 2) {
		print_usage(argv[0]);
		return 1;
	}

	Options options;
	options.mode = argv[1];

	if (options.mode == "server") {
		if (!parse_server_args(argc, argv, options)) {
			print_usage(argv[0]);
			return 1;
		}
		return run_server(options);
	}

	if (options.mode == "client") {
		if (!parse_client_args(argc, argv, options)) {
			print_usage(argv[0]);
			return 1;
		}
		return run_client(options);
	}

	print_usage(argv[0]);
	return 1;
}