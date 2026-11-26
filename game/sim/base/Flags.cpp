#include "game/sim/base/Flags.h"

#include <algorithm>

namespace takeover {

void BitSet::init(int bit) {
    words_.clear();
    ensureWords(static_cast<std::size_t>(bit / BITS_PER_WORD));
    words_.push_back(1 << (bit % BITS_PER_WORD));
}

BitSet BitSet::getCopy() const {
    BitSet copy(0);
    BitSet source = *this;
    copy.unite(source);
    return copy;
}

void BitSet::ensureWords(std::size_t count) {
    while (words_.size() < count) words_.push_back(0);
}

BitSet& BitSet::unite(BitSet& other) {
    ensureWords(other.words_.size());
    other.ensureWords(words_.size());
    for (std::size_t i = 0; i < words_.size(); ++i) words_[i] |= other.words_[i];
    return *this;
}

BitSet& BitSet::subtract(const BitSet& other) {
    for (std::size_t i = 0; i < words_.size(); ++i)
        words_[i] &= ~(i < other.words_.size() ? other.words_[i] : 0);
    return *this;
}

BitSet& BitSet::intersect(BitSet& other) {
    ensureWords(other.words_.size());
    other.ensureWords(words_.size());
    for (std::size_t i = 0; i < words_.size(); ++i) words_[i] &= other.words_[i];
    return *this;
}

bool BitSet::compare(BitSet& other) {
    ensureWords(other.words_.size());
    other.ensureWords(words_.size());
    return words_ == other.words_;
}

bool BitSet::intersects(const BitSet& other) const {
    const std::size_t n = std::min(words_.size(), other.words_.size());
    for (std::size_t i = 0; i < n; ++i)
        if ((words_[i] & other.words_[i]) != 0) return true;
    return false;
}

std::string BitSet::toString() const {
    std::string s;
    for (std::size_t i = 0; i < words_.size(); ++i) s += (i ? "," : "") + std::to_string(words_[i]);
    return s;
}

} // namespace takeover
