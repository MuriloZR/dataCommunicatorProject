#include "parse.hpp"
#include "server.hpp"
#include <iostream>
#include <string>

void print_usage(const char* program) {
	std::cerr
		<< "Uso:\n"
		<< "  " << program << " server [--port N] [--loss P] [--timeout MS] [--window N] [--output FILE]\n"
		<< "  " << program << " client --host IP --port N --file FILE [--timeout MS] [--window N]\n";
}

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