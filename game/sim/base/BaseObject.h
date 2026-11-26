// BaseObject.h - root of the simulation objects (original: base/_SafeCls_871).
//
// Every simulation object gets a unique id and is registered in a global
// table, so objects refer to each other by id and a reference to a
// disposed object simply resolves to nothing. Objects talk through an
// event bus keyed by integer event types:
//
//     static const int EVENT_ATTACK = BaseObject::newEventType();
//     squad->addEventListener(EVENT_ATTACK, getId(), [this](int, BaseObject* from, const std::any& data) {...});
//     squad->dispatchEvent(EVENT_ATTACK, target);
//
// A listener is removed automatically when the listening object is
// disposed. Time: update(dt) advances `activeTime` (only while not paused)
// and `totalTime` (always).
#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>

namespace takeover {

    class BaseObject : public std::enable_shared_from_this<BaseObject> {
    public:
        using Handler = std::function<void(int eventType, BaseObject* sender, const std::any& data)>;

        BaseObject();
        virtual ~BaseObject();
        BaseObject(const BaseObject&) = delete;
        BaseObject& operator=(const BaseObject&) = delete;

        /// Allocates a new event type id (original: _SafeStr_883).
        static int newEventType();

        /// Dispatched by dispose() (original: _SafeStr_880).
        static const int EVENT_DISPOSED;

        /// Restarts object ids and event types (original: _SafeStr_888).
        static void resetCounters();

        /// The live object with this id, or nullptr.
        static BaseObject* getObjectPtr(int id);

        [[nodiscard]] int getId() const { return id_; }               ///< Original: _SafeStr_879.
        [[nodiscard]] double getActiveTime() const { return activeTime_; } ///< Original: _SafeStr_885.
        [[nodiscard]] double getTotalTime() const { return totalTime_; }   ///< Original: _SafeStr_887.
        void setActiveTime(double t) { activeTime_ = t; }              ///< Original: _SafeStr_894.
        void setTotalTime(double t) { totalTime_ = t; }                ///< Original: _SafeStr_892.
        [[nodiscard]] bool isPaused() const { return paused_; }        ///< Original: _SafeStr_886.
        void setPaused(bool paused) { paused_ = paused; }              ///< Original: _SafeStr_884.

        virtual void update(double dt);

        /// Called every update, paused or not (original: _SafeStr_882).
        virtual void alwaysUpdate(double /*dt*/) {}
        virtual void customUpdate(double /*dt*/) {}

        /// Registers `handler` of object `listenerId` for `eventType`. Returns
        /// false if no such object exists.
        bool addEventListener(int eventType, int listenerId, Handler handler);
        void removeEventListener(int eventType, int listenerId);
        [[nodiscard]] bool hasEventListener(int eventType, int listenerId) const; ///< Original: _SafeStr_891.
        void dispatchEvent(int eventType, const std::any& data = {});
        void removeAllListeners(); ///< Original: _SafeStr_889.

        /// Gives the object a new id (original: _SafeStr_890).
        void registerAgain();

        /// Forgets every listener (original: _SafeStr_893).
        void resetListeners() { listeners_.clear(); }
        /// Leaves the global table without disposing (original: _SafeStr_895).
        void unregister();

        void dispose();
        virtual void customDispose() {}

    private:
        void addListenerEntry(int eventType, int listenerId, Handler handler); ///< Original: _SafeStr_878.

        static int nextObjectId_;
        static int nextEventType_;

        int id_ = 0;
        double activeTime_ = 0;
        double totalTime_ = 0;
        bool paused_ = false;

        /// event type -> listener id -> handler. std::map iterates in id order
        /// (the original's hash order is unspecified).
        std::map<int, std::map<int, Handler>> listeners_;
    };

} // namespace takeover
