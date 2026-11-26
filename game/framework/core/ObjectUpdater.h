// ObjectUpdater.h - updates a set of objects until they are disposed
// (original: _SafePkg_29/ObjectUpdater).
#pragma once

#include "game/framework/core/Disposable.h"

#include <memory>
#include <vector>

namespace takeover {

    class ObjectUpdater : public UpdatableObject {
    public:
        void addObject(std::shared_ptr<UpdatableObject> object) {
            objects_.push_back(std::move(object));
            ++count_;
        }

        [[nodiscard]] int count() const { return count_; } ///< _SafeStr_2554

        void update(double timeElapsed) override {
            UpdatableObject::update(timeElapsed);
            int counter = 0;

            // A copy: updating may add objects.
            const auto objects = objects_;
            for (const auto& object : objects) {
                if (object->isDisposed()) {
                    std::erase(objects_, object);
                } else {
                    object->update(timeElapsed);
                    ++counter;
                }
            }

            count_ = counter;
        }

    protected:
        void customDispose() override {
            for (const auto& object : objects_)
                object->dispose();

            objects_.clear();
            UpdatableObject::customDispose();
        }

    private:
        std::vector<std::shared_ptr<UpdatableObject>> objects_; ///< Original: _SafeStr_1615.
        int count_ = 0;                                          ///< Original: _SafeStr_2553.
    };

} // namespace takeover
