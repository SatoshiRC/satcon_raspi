#pragma once

#include <cstdint>
#include <vector>
#include <optional>

namespace satcon {

static constexpr uint8_t START_BYTE = 0x7F;
static constexpr size_t FRAME_PAYLOAD_LEN = 12; // 6 * int16
static constexpr size_t FRAME_LEN = 1 + FRAME_PAYLOAD_LEN;

struct Frame {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
};

// Parse a single frame from buffer[offset..]. Throws std::runtime_error on error.
Frame parse_frame(const std::vector<uint8_t>& buf, size_t offset = 0, bool little_endian = false);

// Scan buffer for next full frame. If found, consumes bytes from the front of
// the buffer and returns the Frame. If not enough bytes, returns std::nullopt.
std::optional<Frame> find_and_parse(std::vector<uint8_t>& buffer, bool little_endian = false);

// Utility used by tests
std::vector<uint8_t> encode_frame(const Frame& f, bool little_endian = false);

} // namespace satcon
