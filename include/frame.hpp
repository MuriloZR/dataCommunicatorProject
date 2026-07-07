#pragma once
#include <cstdint>
#include <optional>
#include <vector>

enum class FrameType : uint8_t {
    DATA = 0x01,
    ACK  = 0x02,
    NAK  = 0x03,
    FIN  = 0x04
};

struct Frame {
    uint8_t  flag_start;   // sempre 0x7E
    uint8_t  src;          // endereço origem
    uint8_t  dst;          // endereço destino
    FrameType type;
    uint16_t  seq;          // número de sequência
    uint16_t length;       // tamanho do payload em bytes
    std::vector<uint8_t> payload;
    uint16_t crc;          // calculado sobre tudo acima
    uint8_t  flag_end;     // sempre 0x7E
};

std::vector<uint8_t> serialize(const Frame& f);
std::optional<Frame> deserialize(const std::vector<uint8_t>& raw);
