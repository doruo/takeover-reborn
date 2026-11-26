#include "game/framework/storage/ByteArray.h"

#include <zlib.h>

#include <bit>
#include <cstring>

namespace takeover {

void ByteArray::write8(std::uint8_t byte) {
    if (position_ < bytes_.size()) bytes_[position_] = byte;
    else bytes_.push_back(byte);
    ++position_;
}

std::uint8_t ByteArray::read8() {
    if (position_ >= bytes_.size()) throw EndOfFile();
    return bytes_[position_++];
}

void ByteArray::writeShort(int value) {
    write8(static_cast<std::uint8_t>(value >> 8));
    write8(static_cast<std::uint8_t>(value));
}

void ByteArray::writeInt(int value) {
    const auto v = static_cast<std::uint32_t>(value);
    for (int shift = 24; shift >= 0; shift -= 8) write8(static_cast<std::uint8_t>(v >> shift));
}

void ByteArray::writeFloat(float value) { writeInt(static_cast<int>(std::bit_cast<std::uint32_t>(value))); }

void ByteArray::writeDouble(double value) {
    const auto v = std::bit_cast<std::uint64_t>(value);
    for (int shift = 56; shift >= 0; shift -= 8) write8(static_cast<std::uint8_t>(v >> shift));
}

void ByteArray::writeUTF(const std::string& value) {
    writeShort(static_cast<int>(value.size()));
    for (const char c : value) write8(static_cast<std::uint8_t>(c));
}

void ByteArray::writeBytes(const ByteArray& source, std::size_t offset, std::size_t length) {
    if (length == 0) length = source.length() - offset;
    for (std::size_t i = 0; i < length; ++i) write8(source.bytes_[offset + i]);
}

int ByteArray::readByte() { return static_cast<std::int8_t>(read8()); }

bool ByteArray::readBoolean() { return read8() != 0; }

int ByteArray::readShort() {
    const auto high = read8();
    const auto low = read8();
    return static_cast<std::int16_t>(static_cast<std::uint16_t>((high << 8) | low));
}

int ByteArray::readInt() {
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v = (v << 8) | read8();
    return static_cast<int>(v);
}

float ByteArray::readFloat() { return std::bit_cast<float>(static_cast<std::uint32_t>(readInt())); }

double ByteArray::readDouble() {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | read8();
    return std::bit_cast<double>(v);
}

std::string ByteArray::readUTF() {
    const auto length = static_cast<std::uint16_t>(readShort());
    std::string s;
    for (int i = 0; i < length; ++i) s.push_back(static_cast<char>(read8()));
    return s;
}

void ByteArray::readBytes(ByteArray& target, std::size_t offset, std::size_t length) {
    if (length == 0) length = bytesAvailable();
    if (length > bytesAvailable()) throw EndOfFile();
    if (target.bytes_.size() < offset + length) target.bytes_.resize(offset + length);
    std::memcpy(target.bytes_.data() + offset, bytes_.data() + position_, length);
    position_ += length;
}

void ByteArray::compress() {
    uLongf size = compressBound(static_cast<uLong>(bytes_.size()));
    std::vector<std::uint8_t> out(size);
    if (::compress(out.data(), &size, bytes_.data(), static_cast<uLong>(bytes_.size())) != Z_OK)
        throw std::runtime_error("ByteArray.compress failed");
    out.resize(size);
    bytes_ = std::move(out);
    position_ = bytes_.size();
}

void ByteArray::uncompress() {
    z_stream stream{};
    if (inflateInit(&stream) != Z_OK) throw std::runtime_error("ByteArray.uncompress failed");
    stream.next_in = bytes_.data();
    stream.avail_in = static_cast<uInt>(bytes_.size());
    std::vector<std::uint8_t> out;
    std::uint8_t chunk[16384];
    int status = Z_OK;
    while (status == Z_OK) {
        stream.next_out = chunk;
        stream.avail_out = sizeof chunk;
        status = inflate(&stream, Z_NO_FLUSH);
        out.insert(out.end(), chunk, chunk + (sizeof chunk - stream.avail_out));
    }
    inflateEnd(&stream);
    if (status != Z_STREAM_END) throw std::runtime_error("ByteArray.uncompress: corrupt data");
    bytes_ = std::move(out);
    position_ = 0;
}

} // namespace takeover
