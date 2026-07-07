#pragma once

#include <cstdint>
#include <vector>

uint16_t crc16(const std::vector<uint8_t>& data);

bool verify_crc(const std::vector<uint8_t>& data, uint16_t received_crc);
