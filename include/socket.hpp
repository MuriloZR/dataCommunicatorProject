#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// Abstrai send/recv com timeout em ms (0 = bloqueante)
int create_server(int port);
int create_client(const char *host, int port);
bool send_bytes(int fd, const std::vector<uint8_t> &data);
bool recv_bytes(int fd, std::vector<uint8_t> &out, size_t n, int timeout_ms);
void close_socket(int fd);
