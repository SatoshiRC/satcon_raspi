#include "parser.hpp"
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace satcon {

static int16_t read_int16(const uint8_t* p, bool little_endian) {
    if (little_endian) {
        return static_cast<int16_t>(p[0] | (p[1] << 8));
    } else {
        return static_cast<int16_t>((p[0] << 8) | p[1]);
    }
}

Frame parse_frame(const std::vector<uint8_t>& buf, size_t offset, bool little_endian) {
    if (offset + FRAME_LEN > buf.size()) {
        throw std::runtime_error("not enough bytes for a frame");
    }
    if (buf[offset] != START_BYTE) {
        throw std::runtime_error("missing start byte");
    }
    const uint8_t* payload = buf.data() + offset + 1;
    Frame f;
    f.ax = read_int16(payload + 0, little_endian);
    f.ay = read_int16(payload + 2, little_endian);
    f.az = read_int16(payload + 4, little_endian);
    f.gx = read_int16(payload + 6, little_endian);
    f.gy = read_int16(payload + 8, little_endian);
    f.gz = read_int16(payload + 10, little_endian);
    return f;
}

std::optional<Frame> find_and_parse(std::vector<uint8_t>& buffer, bool little_endian) {
    // find start
    auto it = std::find(buffer.begin(), buffer.end(), START_BYTE);
    if (it == buffer.end()) {
        buffer.clear();
        return std::nullopt;
    }
    size_t idx = std::distance(buffer.begin(), it);
    if (idx > 0) {
        buffer.erase(buffer.begin(), buffer.begin() + idx);
    }
    if (buffer.size() < FRAME_LEN) return std::nullopt;
    try {
        Frame f = parse_frame(buffer, 0, little_endian);
        buffer.erase(buffer.begin(), buffer.begin() + FRAME_LEN);
        return f;
    } catch (...) {
        // skip this start byte
        buffer.erase(buffer.begin());
        return std::nullopt;
    }
}

std::vector<uint8_t> encode_frame(const Frame& f, bool little_endian) {
    std::vector<uint8_t> out;
    out.push_back(START_BYTE);
    auto push_int16 = [&](int16_t v) {
        if (little_endian) {
            out.push_back(static_cast<uint8_t>(v & 0xFF));
            out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        } else {
            out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            out.push_back(static_cast<uint8_t>(v & 0xFF));
        }
    };
    push_int16(f.ax);
    push_int16(f.ay);
    push_int16(f.az);
    push_int16(f.gx);
    push_int16(f.gy);
    push_int16(f.gz);
    return out;
}

} // namespace satcon
