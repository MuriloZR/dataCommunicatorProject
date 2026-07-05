#include "../include/error.hpp"

uint16_t crc16(const std::vector<uint8_t>& data){
    uint16_t reg{0}, generator{0x1021};
    for (uint8_t byte : data) {
        for (int i = 7; i >= 0; i--) {
            uint8_t msb {static_cast<uint8_t>(reg >> 15)};
            uint8_t bit {static_cast<uint8_t>((byte >> i) & 1)};
            reg <<= 1;
            reg |= bit;
            if (msb) reg ^= generator;
        }
    }

    for (int i = 0; i < 16; i++) {
        uint8_t topo {static_cast<uint8_t>(reg >> 15)};
        reg <<= 1;
        if (topo) reg ^= generator;
    }

    return reg;
}

bool verify_crc(const std::vector<uint8_t>& data, uint16_t received_crc) {
    std::vector<uint8_t> augmented = data;
    augmented.push_back(static_cast<uint8_t>(received_crc >> 8));   // Primeiro byte do
    augmented.push_back(static_cast<uint8_t>(received_crc & 0xFF)); // LSB
    return crc16(augmented) == 0;
}