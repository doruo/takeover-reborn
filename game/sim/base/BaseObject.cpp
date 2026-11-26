#include "game/sim/base/BaseObject.h"

#include "game/framework/core/Trash.h"

#include <vector>

namespace takeover {

    namespace {
        std::map<int, BaseObject*>& registry() {
            static std::map<int, BaseObject*> objects;
            return objects;
        }
    } // namespace

    int BaseObject::nextObjectId_ = 0;
    int BaseObject::nextEventType_ = 0;
    const int BaseObject::EVENT_DISPOSED = BaseObject::newEventType();

    int BaseObject::newEventType() { return nextEventType_++; }

    void BaseObject::resetCounters() {
        nextObjectId_ = 0;
        nextEventType_ = 0;
    }

    BaseObject* BaseObject::getObjectPtr(int id) {
        const auto it = registry().find(id);
        return it == registry().end() ? nullptr : it->second;
    }

    BaseObject::BaseObject() {
        id_ = nextObjectId_++;
        registry()[id_] = this;
    }

    BaseObject::~BaseObject() {
        // Not in the original (the GC never collects a registered object):
        // keep the table free of dangling pointers.
        const auto it = registry().find(id_);
        if (it != registry().end() && it->second == this)
            registry().erase(it);
    }

    void BaseObject::update(double dt) {
        if (!paused_) {
            activeTime_ += dt;
            customUpdate(dt);
        }

        totalTime_ += dt;
        alwaysUpdate(dt);
    }

    bool BaseObject::addEventListener(int eventType, int listenerId, Handler handler) {
        BaseObject* listener = getObjectPtr(listenerId);
        if (listener == nullptr)
            return false;

        // When the listener is disposed, it tells us (original: _SafeStr_881).
        // Its entries are also skipped by dispatchEvent once it is gone.
        listener->addListenerEntry(EVENT_DISPOSED, id_, [this, listenerId](int, BaseObject*, const std::any&) {
            for (auto& [type, handlers] : listeners_)
                handlers.erase(listenerId);
        });

        addListenerEntry(eventType, listenerId, std::move(handler));

        return true;
    }

    void BaseObject::addListenerEntry(int eventType, int listenerId, Handler handler) {
        listeners_[eventType][listenerId] = std::move(handler);
    }

    void BaseObject::removeEventListener(int eventType, int listenerId) {
        const auto it = listeners_.find(eventType);
        if (it != listeners_.end())
            it->second.erase(listenerId);
    }

    bool BaseObject::hasEventListener(int eventType, int listenerId) const {
        const auto it = listeners_.find(eventType);
        return it != listeners_.end() && it->second.contains(listenerId);
    }

    void BaseObject::dispatchEvent(int eventType, const std::any& data) {
        const auto it = listeners_.find(eventType);
        if (it == listeners_.end())
            return;

        // Snapshot the ids: handlers may add or remove listeners.
        std::vector<int> ids;

        for (const auto& [id, handler] : it->second)
            ids.push_back(id);

        for (const int id : ids) {
            auto table = listeners_.find(eventType);
            if (table == listeners_.end())
                return;

            auto entry = table->second.find(id);
            if (entry == table->second.end())
                continue;

            if (getObjectPtr(id) == nullptr) {
                table->second.erase(entry);
                continue;
            }

            Handler handler = entry->second; // copy: the table may change
            handler(eventType, this, data);
        }
    }

    void BaseObject::removeAllListeners() { listeners_.clear(); }

    void BaseObject::registerAgain() {
        id_ = nextObjectId_++;
        registry()[id_] = this;
    }

    void BaseObject::unregister() {
        const auto it = registry().find(id_);
        if (it != registry().end() && it->second == this)
            registry().erase(it);
    }

    void BaseObject::dispose() {
        // Keep ourselves alive while listeners react to our death, and until
        // the next frame (see Disposable::dispose).
        std::shared_ptr<BaseObject> self = weak_from_this().lock();
        releaseLater(self);
        dispatchEvent(EVENT_DISPOSED);
        customDispose();
        listeners_.clear();
        unregister();
    }

} // namespace takeover
