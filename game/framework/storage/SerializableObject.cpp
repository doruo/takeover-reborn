#include "game/framework/storage/SerializableObject.h"

#include "game/framework/core/As3.h"

namespace takeover {

void SerializableObject::add(const std::string& name, int type, Value value) {
    fields_.push_back({name, type, std::move(value)});
}

const SerializableObject::Field* SerializableObject::find(const std::string& name) const {
    for (const Field& f : fields_)
        if (f.name == name) return &f;
    return nullptr;
}

bool SerializableObject::has(const std::string& name) const { return find(name) != nullptr; }

double SerializableObject::getNumber(const std::string& name) const {
    const Field* f = find(name);
    if (!f) return 0;
    if (const auto* d = std::get_if<double>(&f->value)) return *d;
    if (const auto* b = std::get_if<bool>(&f->value)) return *b ? 1 : 0;
    return 0;
}

bool SerializableObject::getBool(const std::string& name) const {
    const Field* f = find(name);
    if (!f) return false;
    if (const auto* b = std::get_if<bool>(&f->value)) return *b;
    if (const auto* d = std::get_if<double>(&f->value)) return *d != 0;
    return false;
}

std::string SerializableObject::getString(const std::string& name) const {
    const Field* f = find(name);
    const auto* s = f ? std::get_if<std::string>(&f->value) : nullptr;
    return s ? *s : std::string();
}

ByteArray SerializableObject::getBytes(const std::string& name) const {
    const Field* f = find(name);
    const auto* b = f ? std::get_if<ByteArray>(&f->value) : nullptr;
    return b ? *b : ByteArray();
}

void SerializableObject::save(ByteArray& data) const {
    data.writeInt(static_cast<int>(fields_.size()));
    for (const Field& f : fields_) {
        data.writeUTF(f.name);
        data.writeInt(f.type);
        const double number = std::holds_alternative<double>(f.value) ? std::get<double>(f.value)
                              : std::holds_alternative<bool>(f.value) ? (std::get<bool>(f.value) ? 1 : 0)
                                                                      : 0;
        switch (f.type) {
        case STRING: data.writeUTF(std::get<std::string>(f.value)); break;
        case BYTE: data.writeByte(as3::toInt(number)); break;
        case SHORT: data.writeShort(as3::toInt(number)); break;
        case FLOAT: data.writeFloat(static_cast<float>(number)); break;
        case DOUBLE: data.writeDouble(number); break;
        case INT: data.writeInt(as3::toInt(number)); break;
        case BOOLEAN: data.writeBoolean(number != 0); break;
        case BYTEARRAY: {
            const ByteArray& bytes = std::get<ByteArray>(f.value);
            data.writeShort(static_cast<int>(bytes.length()));
            data.writeBytes(bytes, 0, bytes.length());
            break;
        }
        default: break;
        }
    }
}

void SerializableObject::load(ByteArray& data) {
    const int count = data.readInt();
    for (int i = 0; i < count; ++i) {
        Field f;
        f.name = data.readUTF();
        f.type = data.readInt();
        switch (f.type) {
        case STRING: f.value = data.readUTF(); break;
        case BYTE: f.value = static_cast<double>(data.readByte()); break;
        case INT: f.value = static_cast<double>(data.readInt()); break;
        case SHORT: f.value = static_cast<double>(data.readShort()); break;
        case FLOAT: f.value = static_cast<double>(data.readFloat()); break;
        case DOUBLE: f.value = data.readDouble(); break;
        case BOOLEAN: f.value = data.readBoolean(); break;
        case BYTEARRAY: {
            const int length = data.readShort();
            ByteArray bytes;
            if (length > 0) data.readBytes(bytes, 0, static_cast<std::size_t>(length));
            f.value = std::move(bytes);
            break;
        }
        default: f.value = 0.0; break;
        }
        fields_.push_back(std::move(f));
    }
}

} // namespace takeover
