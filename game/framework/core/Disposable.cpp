#include "game/framework/core/Disposable.h"

#include "game/framework/core/Trash.h"

namespace takeover {

void Disposable::dispose() {
    if (disposed_) return;
    // A disposed object is often still running (a button disposed by its
    // own click). Flash's garbage collector made that safe; here the object
    // is kept alive until the next frame, whoever drops it meanwhile.
    std::shared_ptr<Disposable> self = weak_from_this().lock();
    releaseLater(self);
    preDispose();
    disposed_ = true;
    signalDisposed_.dispatch(this);
    signalDisposed_.dispose();
    customDispose();
}

void UpdatableObject::update(double timeElapsed) {
    if (paused_ || disposed_) return;
    lifetime_ += timeElapsed;
}

} // namespace takeover
