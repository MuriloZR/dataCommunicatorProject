#include "socket.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <print>

void server() {
  int fd = create_server(5000);
  std::vector<uint8_t> dados;

  if (recv_bytes(fd, dados, 5, 5000) == RecvStatus::Ok) {
		std::print("Servidor recebeu: ");

    for (int b : dados)
      std::cout << b << ' ';

    std::putchar('\n');

    std::vector<uint8_t> resposta = {'O', 'K'};

    send_bytes(fd, resposta);
  } else {
		std::println("Timeout no servidor");
  }

  close_socket(fd);
}

void client() {
  std::this_thread::sleep_for(std::chrono::seconds(1));

  int fd = create_client("127.0.0.1", 5000);

  std::vector<uint8_t> msg = {1, 2, 3, 4, 5};

  send_bytes(fd, msg);

  std::vector<uint8_t> resposta;

  if (recv_bytes(fd, resposta, 2, 5000) == RecvStatus::Ok) {
		std::print("Servidor recebeu: ");

    for (char b : resposta)
      std::cout << b;

    std::putchar('\n');
  } else {
		std::println("Timeout no servidor");
  }

  close_socket(fd);
}

int main() {
  std::thread s(server);
  std::thread c(client);

  s.join();
  c.join();
}
