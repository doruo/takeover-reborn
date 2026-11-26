// TypedKey.h - numbered keys and a store indexed by them (original:
// base/_SafeStr_780 and base/_SafeStr_1508).
//
// A TypedKey has a kind (a string such as "cmbtnt" for combatants) and an
// index: the n-th key created of that kind gets index n. Combatants are
// identified by such keys, and per-combatant data is stored by index.
#pragma once

#include <map>
#include <memory>
#include <string>

namespace takeover {

    class TypedKey {
    public:
        explicit TypedKey(std::string kind);
        virtual ~TypedKey() = default;

        /// Restarts the numbering of a kind (original: _SafeStr_1654).
        static void resetCounter(const std::string& kind);

        [[nodiscard]] const std::string& getKey() const { return kind_; }
        [[nodiscard]] int getIndex() const { return index_; } ///< Original: _SafeStr_1655.
        [[nodiscard]] bool sameKind(const TypedKey& other) const { return kind_ == other.kind_; } ///< _SafeStr_1656
        [[nodiscard]] bool compare(const TypedKey& other) const { return kind_ == other.kind_ && index_ == other.index_; }

    private:
        std::string kind_;
        int index_ = 0;
    };

    /// Objects stored by key index (original: _SafeStr_1508). Note that the
    /// kind is ignored: two keys of different kinds with the same index collide,
    /// as in the original.
    template <typename T>
    class KeyedStorage {
    public:
        void addObject(std::shared_ptr<T> object, const TypedKey& key) { objects_[key.getIndex()] = std::move(object); }
        bool removeObject(const TypedKey& key) { return objects_.erase(key.getIndex()) > 0; } ///< _SafeStr_1736
        [[nodiscard]] T* getObject(const TypedKey* key) const {
            if (key == nullptr)
                return nullptr;

            const auto it = objects_.find(key->getIndex());
            return it == objects_.end() ? nullptr : it->second.get();
        }

        [[nodiscard]] std::shared_ptr<T> getShared(const TypedKey* key) const {
            if (key == nullptr)
                return nullptr;

            const auto it = objects_.find(key->getIndex());
            return it == objects_.end() ? nullptr : it->second;
        }

        /// In index order (original: getStorageIterator).
        [[nodiscard]] const std::map<int, std::shared_ptr<T>>& objects() const { return objects_; }
        void dispose() { objects_.clear(); }

    private:
        std::map<int, std::shared_ptr<T>> objects_;
    };

} // namespace takeover
