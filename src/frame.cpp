#include "frame.hpp"
#include "error.hpp"
#include <cstdlib>

std::vector<uint8_t> serialize(const Frame& f) {
    // monta os dados para CRC (sem flags)
    std::vector<uint8_t> crc_data = {
        f.src, f.dst, static_cast<uint8_t>(f.type), f.seq,
        static_cast<uint8_t>(f.length >> 8),
        static_cast<uint8_t>(f.length & 0xFF)
    };
    for (auto b : f.payload) crc_data.push_back(b);

    uint16_t crc = crc16(crc_data);

    std::vector<uint8_t> bytes;
    bytes.push_back(0x7E);                          // flag_start
    for (auto b : crc_data) bytes.push_back(b);    // campos + payload
    bytes.push_back(crc >> 8);                      // CRC MSB
    bytes.push_back(crc & 0xFF);                    // CRC LSB
    bytes.push_back(0x7E);                          // flag_end

    return bytes;
}

Frame deserialize(const std::vector<uint8_t>& raw) {
	if (raw.size() < 10) exit(1);

	Frame f{};
	if (raw.front() != 0x7E || raw.back() != 0x7E) exit(1);
	f.flag_start = raw.front();
	f.flag_end = raw.back();

	f.src = raw[1];
	f.dst = raw[2];
	f.type = (FrameType)raw[3];
	f.seq = raw[4];

	f.length = (raw[5] << 8) | raw[6];

	if (raw.size() != 10 + f.length) exit(1);

	for (int i = 7; i < 7 + f.length; i++) {
		f.payload.push_back(raw[i]);
	}

	uint16_t crc = (raw[7+f.length]<< 8) | raw[8+f.length];
	f.crc = crc;

	std::vector<uint8_t> crc_data(
    raw.begin(),
    raw.begin() + (7 + f.length)
	);
	uint16_t computed_crc = crc16(crc_data);

	if (computed_crc != crc) exit(1);

	return f;
}
