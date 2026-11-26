// Listener.h - callbacks that can be removed again, like AS3 functions.
//
// ActionScript code registers listeners as method closures
// (`signal.addListener(this.onClick)`) and later removes them with an
// equal closure (`signal.removeListener(this.onClick)`). Two closures are
// equal when they bind the same method to the same object.
//
// std::function has no equality, so a Listener carries an explicit key:
// (object address, bytes of the member function pointer). Lambdas get a
// unique key instead and are removed with the key returned when they were
// added.
#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <utility>

namespace engine {

/// Identity of a listener. Equal keys mean "the same function" in the AS3
/// sense.
class ListenerKey {
public:
    ListenerKey() = default;

    /// Key of the method closure `object.method`.
    template <typename Object, typename Method>
    static ListenerKey of(const Object* object, Method method) {
        static_assert(sizeof(Method) <= kMethodBytes, "member function pointer larger than expected");
        ListenerKey key;
        key.object_ = object;
        std::memcpy(key.method_.data(), &method, sizeof(Method));
        return key;
    }

    /// A fresh key nobody else has (for lambdas).
    static ListenerKey unique() {
        static std::uintptr_t counter = 0;
        ListenerKey key;
        key.serial_ = ++counter;
        return key;
    }

    [[nodiscard]] const void* object() const { return object_; }
    [[nodiscard]] bool isNull() const { return object_ == nullptr && serial_ == 0; }

    friend bool operator==(const ListenerKey& a, const ListenerKey& b) {
        return a.object_ == b.object_ && a.serial_ == b.serial_ && a.method_ == b.method_;
    }

private:
    // Member function pointers are one or two machine words (Itanium ABI)
    // and up to four on MSVC with virtual inheritance.
    static constexpr std::size_t kMethodBytes = 4 * sizeof(void*);

    const void* object_ = nullptr;
    std::uintptr_t serial_ = 0;
    std::array<unsigned char, kMethodBytes> method_{};
};

/// A callable plus its identity.
template <typename... Args>
struct Listener {
    ListenerKey key;
    std::function<void(Args...)> call;

    /// `Listener<int>::method(this, &Screen::onButton)`
    template <typename Object, typename Result, typename... Params>
    static Listener method(Object* object, Result (Object::*fn)(Params...)) {
        return {ListenerKey::of(object, fn),
                [object, fn](Args... args) { (object->*fn)(std::forward<Args>(args)...); }};
    }

    /// A lambda with a unique key.
    template <typename Callable>
    static Listener function(Callable&& callable) {
        return {ListenerKey::unique(), std::function<void(Args...)>(std::forward<Callable>(callable))};
    }
};

} // namespace engine
