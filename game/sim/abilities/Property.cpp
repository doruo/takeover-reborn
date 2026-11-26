#include "game/sim/abilities/Property.h"

#include <vector>

namespace takeover {

    Property::Property(std::string name) : propertyId_(nextPropertyId_++), name_(std::move(name)) {}

    void PropertyContainer::removeProperty(std::shared_ptr<IProperty> property) {
        properties_.erase(property->getPropertyId());

        // Iterate over a snapshot: the original's IntMap iterator does too.
        std::vector<std::shared_ptr<IProperty>> others;

        for (auto& [id, p] : properties_)
            others.push_back(p);

        for (auto& other : others)
            other->onPropertyRemoved(*property);

        property->disposeProperty();
    }

    bool PropertyContainer::addProperty(std::shared_ptr<IProperty> property) {
        std::vector<std::shared_ptr<IProperty>> existing;

        for (auto& [id, p] : properties_)
            existing.push_back(p);

        for (auto& other : existing) {
            const bool collided = other->makeAddPropertyCollision(*property);
            if (property->isFinished() || collided)
                return false;

            if (property->makeAddPropertyCollision(*other))
                return false;
        }

        properties_[property->getPropertyId()] = std::move(property);
        return true;
    }

    void PropertyContainer::customUpdate(double dt) {
        std::vector<std::shared_ptr<IProperty>> snapshot;

        for (auto& [id, p] : properties_)
            snapshot.push_back(p);

        for (auto& p : snapshot)
            p->updateProperty(dt);

        snapshot.clear();

        for (auto& [id, p] : properties_)
            snapshot.push_back(p);

        for (auto& p : snapshot)
            if (p->isFinished())
                removeProperty(p);
        BaseObject::customUpdate(dt);
    }

    void PropertyContainer::customDispose() {
        auto properties = std::move(properties_);
        properties_.clear();

        for (auto& [id, p] : properties)
            p->disposeProperty();

        BaseObject::customDispose();
    }

} // namespace takeover
