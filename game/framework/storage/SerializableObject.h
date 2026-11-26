// SerializableObject.h - a list of named, typed fields written to a
// ByteArray (original: _SafePkg_1650/SerializableObject, with the type
// codes of _SafePkg_1650/_SafeStr_2175).
//
// Layout: int field count, then per field: UTF name, int type, value.
// Reading back by name lets newer versions skip or add fields.
#pragma once

#include "game/framework/storage/ByteArray.h"

#include <string>
#include <variant>
#include <vector>

namespace takeover {

    class SerializableObject {
    public:
        // Field types (original: _SafeStr_2175).
        static constexpr int STRING = 1;
        static constexpr int BYTE = 2;
        static constexpr int INT = 3;
        static constexpr int SHORT = 4;
        static constexpr int FLOAT = 5;
        static constexpr int DOUBLE = 6;
        static constexpr int BOOLEAN = 7;
        static constexpr int BYTEARRAY = 20;

        using Value = std::variant<double, bool, std::string, ByteArray>;

        /// Adds a field (original: _SafeStr_2179). Numbers are written with the
        /// precision of `type` (SHORT truncates to 16 bits, as in Flash).
        void add(const std::string& name, int type, Value value);
        [[nodiscard]] bool has(const std::string& name) const; ///< _SafeStr_2178

        /// The value as the original's dynamic typing would read it.
        [[nodiscard]] double getNumber(const std::string& name) const;
        [[nodiscard]] bool getBool(const std::string& name) const;
        [[nodiscard]] std::string getString(const std::string& name) const;
        [[nodiscard]] ByteArray getBytes(const std::string& name) const;

        void save(ByteArray& data) const;
        void load(ByteArray& data);

    private:
        struct Field {
            std::string name;
            int type = 0;
            Value value;
        };

        [[nodiscard]] const Field* find(const std::string& name) const;
        std::vector<Field> fields_;
    };

} // namespace takeover
