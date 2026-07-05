#include "socket.hpp"
#include <chrono>

int create_server(int port) {
	int server = socket(AF_INET, SOCK_STREAM, 0);

	if (server == -1) {
		std::println("Erro fatal: Falha ao inicializar o socket com erro {}", std::strerror(errno));
		std::exit(-1);
	}

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;

	int opt{1};
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	int serverBind = bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address));

	if (serverBind == -1) {
		std::println(std::cerr, "Erro fatal: Falha ao bindar socket com o erro {}", std::strerror(errno));
		std::exit(-1);
	}

	int serverListen = listen(server, 5);

	if (serverListen == -1) {
		std::println(std::cerr, "Erro fatal: Falha ao escutar socket com o erro {}", std::strerror(errno));
		std::exit(-1);
	}

	int clientSocket = accept(server, nullptr, nullptr);

	if (clientSocket == -1) {
		std::println(std::cerr, "Erro fatal: Falha ao aceitar conexao com o erro {}", std::strerror(errno));
		std::exit(-1);
	}

	close_socket(server);

	return clientSocket;
}

int create_client(const char* host, int port) {
	int client = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in server{};
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if (inet_pton(AF_INET, host, &server.sin_addr) != 1) {
		std::println(std::cerr, "Erro fatal: Endereco invalido");
		std::exit(-1);
	}

	int connection = connect(client, reinterpret_cast<sockaddr*>(&server), sizeof(server));

	if (connection == -1) {
		std::println(std::cerr, "Erro fatal: Falha na conexao com o erro {}", std::strerror(errno));
		std::exit(-1);
	}

	return client;
}

bool send_bytes(int fd, const std::vector<uint8_t>& data) {
	size_t totalEnviados{};
	size_t tam{data.size()};

	while (totalEnviados < tam) {
		int enviados = send(fd, data.data() + totalEnviados, data.size() - totalEnviados, 0);
		if (enviados <= 0)
			return false;

		totalEnviados += enviados;
	}
	return true;
}

bool recv_bytes(int fd, std::vector<uint8_t>& out, size_t n, int timeout_ms) {
    out.resize(n);
    size_t totalRecebidos{};

    // marca o instante inicial para calcular tempo restante a cada iteração
    auto inicio = std::chrono::steady_clock::now();

    while (totalRecebidos < n) {
        if (timeout_ms > 0) {
            auto agora = std::chrono::steady_clock::now();
            int decorrido = std::chrono::duration_cast<std::chrono::milliseconds>
                            (agora - inicio).count();
            int restante = timeout_ms - decorrido;

            if (restante <= 0) return false; // tempo total esgotado

            pollfd pfd{};
            pfd.fd = fd;
            pfd.events = POLLIN;

            int pronto = poll(&pfd, 1, restante);

            if (pronto <= 0) return false;
            if (!(pfd.revents & POLLIN)) return false;
        }

        int recebidos = recv(fd, out.data() + totalRecebidos, n - totalRecebidos, 0);

        if (recebidos <= 0) return false;

        totalRecebidos += recebidos;
    }
    return true;
}

void close_socket(int fd) {
	close(fd);
}
