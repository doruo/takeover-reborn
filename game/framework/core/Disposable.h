// Disposable.h - explicit end of life, and per-frame updates.
//
// Original: _SafePkg_33/_SafeCls_32 (Disposable) and _SafePkg_33/_SafeStr_36
// (UpdatableObject), with the interfaces of _SafePkg_29.
//
// In the original, dispose() releases what the object holds and announces
// it with a signal; the garbage collector frees the memory later. Here,
// dispose() does the same, and memory is freed when the last shared_ptr
// goes away, but never before the next frame: dispose() puts the object in
// the trash (Trash.h), because disposed objects are often still running.
#pragma once

#include "game/framework/core/Signal.h"

#include <memory>

namespace takeover {

class Disposable : public std::enable_shared_from_this<Disposable> {
public:
    Disposable() = default;
    virtual ~Disposable() = default;
    Disposable(const Disposable&) = delete;
    Disposable& operator=(const Disposable&) = delete;

    /// Runs preDispose(), marks the object, tells the listeners, then runs
    /// customDispose(). Calling it again does nothing.
    void dispose();

    [[nodiscard]] bool isDisposed() const { return disposed_; }
    /// Dispatched with the object being disposed (original: _SafeStr_30).
    [[nodiscard]] Signal<Disposable*>& signalDisposed() { return signalDisposed_; }

protected:
    virtual void preDispose() {}
    virtual void customDispose() {}

    bool disposed_ = false;

private:
    Signal<Disposable*> signalDisposed_;
};

/// A Disposable that is updated every frame and can be paused; it counts
/// the time it has been running (original: _SafeStr_36).
class UpdatableObject : public Disposable {
public:
    [[nodiscard]] bool isPaused() const { return paused_; }
    [[nodiscard]] double lifetime() const { return lifetime_; }
    virtual void pause() { paused_ = true; }
    virtual void resume() { paused_ = false; }

    /// Override and call the base version: it does nothing while paused or
    /// disposed, else adds `timeElapsed` to the lifetime.
    virtual void update(double timeElapsed);

protected:
    double lifetime_ = 0;
    bool paused_ = false;
};

} // namespace takeover
