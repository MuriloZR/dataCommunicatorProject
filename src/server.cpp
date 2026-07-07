#include "server.hpp"
#include "arq.hpp"
#include "parse.hpp"
#include "socket.hpp"
#include <iostream>

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