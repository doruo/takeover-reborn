// BitReader.h - sequential reader for the SWF binary encoding.
//
// SWF mixes two encodings in the same byte stream:
//   * byte-aligned little-endian integers (UI8, UI16, SI32, FIXED8, ...),
//   * bit-packed big-endian-within-byte fields (UB[n], SB[n], FB[n]) used by
//     RECT, MATRIX, CXFORM and the shape records.
// Reading any byte-aligned value implicitly discards the remaining bits of the
// current byte, exactly as the SWF specification requires.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>

namespace swf {

/// Thrown when a read goes past the end of the buffer. A truncated or
/// malformed file is a hard error for the converter: we never guess.
class FormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class BitReader {
public:
    BitReader() = default;
    explicit BitReader(std::span<const std::uint8_t> data) : data_(data) {}

    // --- Position -----------------------------------------------------------

    [[nodiscard]] std::size_t bytePosition() const { return bitPos_ >> 3; }
    [[nodiscard]] std::size_t size() const { return data_.size(); }
    [[nodiscard]] std::size_t remaining() const { return data_.size() - bytePosition(); }
    [[nodiscard]] bool atEnd() const { return bytePosition() >= data_.size(); }

    /// Moves to the next byte boundary (no-op if already aligned).
    void align() { bitPos_ = (bitPos_ + 7) & ~std::size_t{7}; }

    void seek(std::size_t bytePos) { bitPos_ = bytePos << 3; }
    void skip(std::size_t bytes) {
        align();
        require(bytes);
        bitPos_ += bytes << 3;
    }

    // --- Byte-aligned values --------------------------------------------------

    std::uint8_t u8() {
        align();
        require(1);
        return data_[advance(1)];
    }

    std::uint16_t u16() {
        align();
        require(2);
        const std::size_t i = advance(2);
        return static_cast<std::uint16_t>(data_[i] | (data_[i + 1] << 8));
    }

    std::uint32_t u32() {
        align();
        require(4);
        const std::size_t i = advance(4);
        return static_cast<std::uint32_t>(data_[i]) | (static_cast<std::uint32_t>(data_[i + 1]) << 8) |
               (static_cast<std::uint32_t>(data_[i + 2]) << 16) | (static_cast<std::uint32_t>(data_[i + 3]) << 24);
    }

    std::int16_t s16() { return static_cast<std::int16_t>(u16()); }
    std::int32_t s32() { return static_cast<std::int32_t>(u32()); }

    /// 8.8 signed fixed point (used by e.g. the blur radius of filters).
    float fixed8() { return static_cast<float>(s16()) / 256.0f; }
    /// 16.16 signed fixed point.
    double fixed16() { return static_cast<double>(s32()) / 65536.0; }

    float f32() {
        const std::uint32_t bits = u32();
        float value;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    /// Variable-length encoded unsigned 32-bit integer (EncodedU32).
    std::uint32_t encodedU32() {
        std::uint32_t result = 0;
        for (int i = 0; i < 5; ++i) {
            const std::uint8_t byte = u8();
            result |= static_cast<std::uint32_t>(byte & 0x7F) << (7 * i);
            if ((byte & 0x80) == 0) break;
        }
        return result;
    }

    /// Null-terminated string. SWF 6+ strings are UTF-8.
    std::string string() {
        align();
        std::string result;
        for (;;) {
            const std::uint8_t c = u8();
            if (c == 0) break;
            result.push_back(static_cast<char>(c));
        }
        return result;
    }

    /// Returns a view on the next `count` bytes and advances past them.
    std::span<const std::uint8_t> bytes(std::size_t count) {
        align();
        require(count);
        return data_.subspan(advance(count), count);
    }

    /// Everything from the current (aligned) position to the end.
    std::span<const std::uint8_t> rest() { return bytes(remaining()); }

    // --- Bit-packed values ----------------------------------------------------

    /// Unsigned bit field, most significant bit first.
    std::uint32_t ub(unsigned bitCount) {
        if (bitCount == 0) return 0;
        if (bitCount > 32) throw FormatError("bit field wider than 32 bits");
        if (bitPos_ + bitCount > data_.size() * 8) throw FormatError("unexpected end of SWF data (bits)");
        std::uint32_t value = 0;
        for (unsigned i = 0; i < bitCount; ++i) {
            const std::uint8_t byte = data_[bitPos_ >> 3];
            const unsigned bit = (byte >> (7 - (bitPos_ & 7))) & 1u;
            value = (value << 1) | bit;
            ++bitPos_;
        }
        return value;
    }

    /// Signed (two's complement) bit field.
    std::int32_t sb(unsigned bitCount) {
        if (bitCount == 0) return 0;
        const std::uint32_t raw = ub(bitCount);
        const std::uint32_t signBit = 1u << (bitCount - 1);
        return static_cast<std::int32_t>((raw ^ signBit) - signBit);
    }

    /// 16.16 fixed point stored as a signed bit field.
    double fb(unsigned bitCount) { return static_cast<double>(sb(bitCount)) / 65536.0; }

    bool flag() { return ub(1) != 0; }

private:
    void require(std::size_t bytes) const {
        if (bytePosition() + bytes > data_.size()) throw FormatError("unexpected end of SWF data");
    }
    std::size_t advance(std::size_t bytes) {
        const std::size_t start = bitPos_ >> 3;
        bitPos_ += bytes << 3;
        return start;
    }

    std::span<const std::uint8_t> data_;
    std::size_t bitPos_ = 0;
};

} // namespace swf
