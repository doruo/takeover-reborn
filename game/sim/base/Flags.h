// Flags.h - small bit-flag helpers (original: base/_SafeStr_694 and
// base/_SafeStr_2742).
#pragma once

#include <string>
#include <vector>

namespace takeover {

/// Bit flags in one int (original: _SafeStr_694).
class Flags {
public:
    explicit Flags(int value = 0) : value_(value) {}

    static bool test(int value, int bits) { return (value & bits) != 0; } ///< _SafeStr_2807
    void set(int value) { value_ = value; }                               ///< _SafeStr_2809
    void clear(int bits) { value_ &= ~bits; }                             ///< _SafeStr_2810
    [[nodiscard]] int get() const { return value_; }                      ///< _SafeStr_833
    [[nodiscard]] bool test(int bits) const { return (value_ & bits) != 0; } ///< _SafeStr_2808
    void add(int bits) { value_ |= bits; }                                ///< _SafeStr_2806

private:
    int value_;
};

/// A set of bit numbers stored 10 per word (original: _SafeStr_2742). Used
/// to describe which ability properties a unit has.
class BitSet {
public:
    static constexpr int BITS_PER_WORD = 10;

    /// The set containing only bit `bit` (bit 0 = "empty" for the original).
    explicit BitSet(int bit = 0) { init(bit); }

    void init(int bit);
    [[nodiscard]] BitSet getCopy() const;
    BitSet& unite(BitSet& other);      ///< In-place or (original: _SafeStr_2766).
    BitSet& subtract(const BitSet& other); ///< In-place and-not (original: _SafeStr_2774).
    BitSet& intersect(BitSet& other);  ///< In-place and (original: _SafeStr_2165).
    bool compare(BitSet& other);
    [[nodiscard]] bool intersects(const BitSet& other) const; ///< Original: _SafeStr_2770.
    [[nodiscard]] std::string toString() const;

private:
    void ensureWords(std::size_t count); ///< Original: _SafeStr_2798.
    std::vector<int> words_;
};

} // namespace takeover
