// ByteArray.h - flash.utils.ByteArray: a growable byte buffer with a read
// position, big-endian like Flash's default.
//
// Saves are written with it exactly as the original did (writeUTF is a
// 16-bit length followed by UTF-8 bytes, ints are big-endian), so the save
// format of the port is the original's.
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace takeover {

class ByteArray {
public:
    ByteArray() = default;
    explicit ByteArray(std::vector<std::uint8_t> bytes) : bytes_(std::move(bytes)) {}

    [[nodiscard]] std::size_t length() const { return bytes_.size(); }
    [[nodiscard]] std::size_t position() const { return position_; }
    void setPosition(std::size_t position) { position_ = position; }
    [[nodiscard]] std::size_t bytesAvailable() const { return position_ < bytes_.size() ? bytes_.size() - position_ : 0; }
    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const { return bytes_; }
    void clear() { bytes_.clear(), position_ = 0; }

    void writeByte(int value) { write8(static_cast<std::uint8_t>(value)); }
    void writeBoolean(bool value) { write8(value ? 1 : 0); }
    void writeShort(int value);
    void writeInt(int value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeUTF(const std::string& value);
    void writeBytes(const ByteArray& source, std::size_t offset = 0, std::size_t length = 0);

    int readByte();         ///< Signed, like Flash.
    bool readBoolean();
    int readShort();        ///< Signed.
    int readInt();
    float readFloat();
    double readDouble();
    std::string readUTF();
    /// Reads `length` bytes into `target` at `offset` (0 = all available).
    void readBytes(ByteArray& target, std::size_t offset = 0, std::size_t length = 0);

    /// zlib compression of the whole buffer (ByteArray.compress / uncompress).
    void compress();
    void uncompress();

    /// Thrown when reading past the end (Flash's EOFError).
    struct EndOfFile : std::runtime_error {
        EndOfFile() : std::runtime_error("ByteArray: read past the end") {}
    };

private:
    void write8(std::uint8_t byte);
    std::uint8_t read8();

    std::vector<std::uint8_t> bytes_;
    std::size_t position_ = 0;
};

} // namespace takeover
