#include "game/framework/core/Trash.h"

#include <vector>

namespace takeover {

namespace {

std::vector<std::shared_ptr<void>>& trash() {
    static std::vector<std::shared_ptr<void>> items;
    return items;
}

} // namespace

void releaseLater(std::shared_ptr<void> object) {
    if (object) trash().push_back(std::move(object));
}

void emptyTrash() {
    // Destructors may release more objects: loop until it stays empty.
    while (!trash().empty()) {
        std::vector<std::shared_ptr<void>> batch;
        batch.swap(trash());
        batch.clear();
    }
}

} // namespace takeover
