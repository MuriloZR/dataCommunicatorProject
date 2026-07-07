#pragma once

#include "frame.hpp"
#include "server.hpp"
#include <cstdint>
#include <string>
#include <vector>

bool parse_int(const std::string& text, int& value);

bool parse_uint8(const std::string& text, uint8_t& value);

bool parse_float(const std::string& text, float& value);

bool parse_server_args(int argc, char** argv, Options& options);

bool parse_client_args(int argc, char** argv, Options& options);

bool read_file_bytes(const std::string& path, std::vector<uint8_t>& data);

bool write_file_bytes(const std::string& path, const std::vector<uint8_t>& data);

std::string encode_hex(const std::vector<uint8_t>& input);

bool decode_hex(const std::vector<uint8_t>& encoded, std::vector<uint8_t>& output);

Frame make_data_frame(uint16_t seq, std::vector<uint8_t> payload);

std::vector<Frame> build_frames_from_file(const std::string& path);