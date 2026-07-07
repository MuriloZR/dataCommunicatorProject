#pragma once

#include <cstdint>
#include <string>

constexpr uint8_t kClientAddress = 1;
constexpr uint8_t kServerAddress = 2;
constexpr std::size_t kPayloadChunkSize = 128;

struct Options {
	std::string mode;
	std::string host{"127.0.0.1"};
	std::string input_file;
	std::string output_file{"recebido"};
	int port{5000};
	uint8_t window_size{4};
	int timeout_ms{3000};
	float loss_prob{0.0f};
};

int run_server(const Options& options);

int run_client(const Options& options);