#pragma once

#include "frame.hpp"
#include <cstdint>
#include <vector>

struct GBNConfig {
    uint8_t  window_size;    // máximo 2^(n-1) onde n = bits do campo seq
                             // com seq de 8 bits, máximo = 127
    int      timeout_ms;     // tempo até retransmissão da janela inteira
    float    loss_prob;      // probabilidade de descarte no receptor [0.0, 1.0]
};

// Envia todos os quadros em `frames` usando Go-Back-N.
// Bloqueia até todos os quadros serem confirmados ou
// o número máximo de retransmissões ser atingido.
// Retorna true se todos os quadros foram confirmados.
bool gbn_sender(int fd,
                const std::vector<Frame>& frames,
                const GBNConfig& cfg);

// Recebe quadros usando Go-Back-N até receber um quadro
// com type == FrameType::DATA e payload vazio (sinal de fim),
// ou até o socket fechar.
// Os quadros recebidos em ordem são adicionados a `out`.
void gbn_receiver(int fd,
                  const GBNConfig& cfg,
                  std::vector<Frame>& out);