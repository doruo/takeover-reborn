#include "game/gui/BadgesRoomScreen.h"

#include "game/framework/Symbols.h"
#include "game/framework/core/As3.h"
#include "game/progress/Profile.h"

namespace takeover {

BadgesRoomScreen::BadgesRoomScreen(std::shared_ptr<GameScreenDef> def) : profile_(def->profile) {
    def->viewClass = "scrBadgesRoomClass";
    initScreen(std::move(def));
    createPages();
}

void BadgesRoomScreen::initVisualBlock() {
    GameScreen::initVisualBlock();
    static_cast<engine::MovieClip*>(image_.get())->gotoAndStop(profile_->selectedCompany().info().number() + 1);
    statPanel_ = std::make_unique<PlayerStatPanel>(clip("_-dj"), *profile_);
}

void BadgesRoomScreen::disposeVisualBlock() {
    statPanel_.reset();
    GameScreen::disposeVisualBlock();
}

void BadgesRoomScreen::initButtonsBlock() {
    GameScreen::initButtonsBlock();
    addButton("butOk", "OK", BUT_OK);
    scrollLeft_ = addButton("_-FM", std::nullopt, BUT_SCROLL_LEFT);  // _SafeStr_2095
    scrollRight_ = addButton("_-lo", std::nullopt, BUT_SCROLL_RIGHT); // _SafeStr_2096
}

void BadgesRoomScreen::disposeButtonsBlock() {
    scrollLeft_ = scrollRight_ = nullptr;
    GameScreen::disposeButtonsBlock();
}

void BadgesRoomScreen::onButtonClick(Button* button) {
    if (button->id() == BUT_SCROLL_LEFT) showPage(currentPage_ - 1);
    else if (button->id() == BUT_SCROLL_RIGHT) showPage(currentPage_ + 1);
    GameScreen::onButtonClick(button);
}

void BadgesRoomScreen::createPages() {
    std::shared_ptr<engine::MovieClip> page;
    int pageBadgeCount = 0;
    for (const std::string& id : Badges::badgeIds()) {
        const Badge* badge = profile_->badges().badge(id);
        if (!page || pageBadgeCount >= 8) {
            page = symbols::create("AchievementsPageClass");
            pages_.push_back(page);
            pageBadgeCount = 0;
            for (int slot = 1; slot <= 8; ++slot) page->getChildByName("badge" + std::to_string(slot))->setVisible(false);
        }
        ++pageBadgeCount;
        auto* slot = symbols::child(*page, "badge" + std::to_string(pageBadgeCount));
        slot->setVisible(true);
        showBadge(slot, *badge);
    }

    pageIndicators_ = std::make_shared<engine::Sprite>();
    pageIndicators_->setX(515);
    pageIndicators_->setY(45);
    image_->addChild(pageIndicators_);
    double x = -0.5 * 15 * (static_cast<double>(pages_.size()) - 1);
    for (std::size_t p = 0; p < pages_.size(); ++p) {
        auto indicator = symbols::create("PageIndicatorClass");
        indicator->setName("page" + std::to_string(p));
        indicator->setX(x);
        indicator->gotoAndStop("_deselected");
        pageIndicators_->addChild(indicator);
        x += 15;
    }
    image_->addChild(pages_[0]);
    currentPage_ = 0;
    scrollLeft_->image()->setVisible(false);
    auto* first = symbols::child(*pageIndicators_, "page0");
    first->gotoAndStop("_selected");
    symbols::setHtmlText(*first, "_-Ys", "1"); // _-Ys: page number (_SafeStr_2804)
}

void BadgesRoomScreen::showBadge(engine::MovieClip* slot, const Badge& badge) {
    auto* progress = symbols::child<engine::TextField>(*slot, "txtProgress");
    if (badge.condition()->fulfilled()) {
        slot->gotoAndStop("_unlocked");
        progress = symbols::child<engine::TextField>(*slot, "txtProgress");
        if (progress) progress->setVisible(false);
    } else {
        slot->gotoAndStop("_locked");
        progress = symbols::child<engine::TextField>(*slot, "txtProgress");
    }
    symbols::child(*slot, "icon")->gotoAndStop(static_cast<int>(badge.iconNum()));
    symbols::setHtmlText(*slot, "txtName", badge.name());
    symbols::setHtmlText(*slot, "txtDesc", badge.description());
    const auto* condition = static_cast<const MetricCondition*>(badge.condition()->condition());
    const double value = condition->metric()->value();
    if (progress) progress->setHtmlText(as3::toString(value) + " / " + as3::toString(condition->constant()));
    engine::MovieClip* bar = symbols::child(*slot, "bar");
    bar->gotoAndStop(as3::toInt(1 + (bar->totalFrames() - 1) * value / condition->constant()));
}

void BadgesRoomScreen::showPage(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= static_cast<int>(pages_.size())) return;
    image_->removeChild(pages_[static_cast<std::size_t>(currentPage_)].get());
    image_->addChild(pages_[static_cast<std::size_t>(pageIndex)]);
    scrollLeft_->image()->setVisible(pageIndex > 0);
    scrollRight_->image()->setVisible(pageIndex < static_cast<int>(pages_.size()) - 1);
    symbols::child(*pageIndicators_, "page" + std::to_string(currentPage_))->gotoAndStop("_deselected");
    auto* next = symbols::child(*pageIndicators_, "page" + std::to_string(pageIndex));
    next->gotoAndStop("_selected");
    symbols::setHtmlText(*next, "_-Ys", std::to_string(pageIndex + 1));
    currentPage_ = pageIndex;
}

} // namespace takeover
