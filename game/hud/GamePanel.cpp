#include "game/hud/GamePanel.h"

#include "game/framework/Symbols.h"
#include "game/framework/core/As3.h"
#include "game/framework/core/Format.h"
#include "game/framework/ui/HintWindow.h"
#include "game/level/Level.h"
#include "game/sim/battle/Combatant.h"
#include "game/sim/spells/SpellInfo.h"
#include "game/squads/SquadIcon.h"

#include "engine/display/MovieClip.h"
#include "engine/display/TextField.h"
#include "engine/events/Event.h"

#include <algorithm>
#include <cmath>

namespace takeover {

namespace {

constexpr int HINT_NONE = 0;
constexpr int HINT_GOLD = 1;
constexpr int HINT_SUPPLY = 2;
constexpr int HINT_MANA = 3;

std::shared_ptr<engine::MovieClip> shared(engine::DisplayObject* object) {
    return object ? std::dynamic_pointer_cast<engine::MovieClip>(object->shared_from_this()) : nullptr;
}

} // namespace

// --- USpellButton ------------------------------------------------------------------------------------

USpellButton::USpellButton(ButtonDef def) {
    init(std::move(def));
    iconBack_ = symbols::child(*image(), "mcIconBack");
    setReadOnly(false);
}

void USpellButton::setReadyFactor(double factor) {
    if (factor < 1) {
        if (isClickable()) setReadOnly(true);
    } else if (!isClickable()) {
        setReadOnly(false);
    }
    iconBack_->gotoAndStop(static_cast<int>(as3::round(iconBack_->totalFrames() * factor)) + 1);
}

// --- GamePanel -----------------------------------------------------------------------------------------

void GamePanel::showSpellHint(int spellId, Level& level) {
    fillSpellHint(Spell(*level.playerSpell(spellId), &level.player().bonuses),
                  *level.hintLayer().getHintWindow(level.hintSpellId()));
}

void GamePanel::fillSpellHint(const Spell& spell, HintWindow2& hint) {
    std::string text = "<br><br>";
    symbols::setHtmlText(*hint.image(), "_-7O", spell.name); // the hint title (_SafeStr_3040)
    switch (spell.type) {
    case SpellInfo::TYPE_BANNER: text += "<FONT COLOR='#999999'>BANNER - " + as3::toString(spell.time) + " sec.</FONT>"; break;
    case SpellInfo::TYPE_SPELL: text += "<FONT COLOR='#999999'>SPELL</FONT>"; break;
    case SpellInfo::TYPE_ULTIMATE: text += "<FONT COLOR='#999999'>ULTIMATE POWER</FONT>"; break;
    }
    text += "<br><br><FONT SIZE='+1'>" + spell.hint + "</FONT>";
    hint.setText(text);
}

GamePanel& GamePanel::init(Level& level) {
    level_ = &level;
    createPanel();
    enabled_ = true;
    pendingMenuButton_ = -1;
    pendingSpellButton_ = -1;
    return *this;
}

void GamePanel::customDispose() {
    signalSpellButton.dispose();
    signalMenuButton.dispose();
    signalUltimate.dispose();
    destroyPanel();
    UpdatableObject::customDispose();
}

void GamePanel::update(double timeElapsed) {
    UpdatableObject::update(timeElapsed);
    symbols::setHtmlText(*panel_, "_-aN.txtTime", format::time(lifetime(), false, true, true, false));
    const char* rank = lifetime() <= GOLD_TIME ? "PERFECT" : lifetime() <= SILVER_TIME ? "GREAT" : "GOOD";
    symbols::setHtmlText(*panel_, "mcRank.txtRank", rank);
}

bool GamePanel::process(double, bool locked) {
    bool lock = locked;
    if (!lock) {
        if (pendingMenuButton_ >= 0) {
            signalMenuButton.dispatch(pendingMenuButton_);
            if (pendingMenuButton_ == BUT_FAST || pendingMenuButton_ == BUT_FAST_ON) swapSpeedButtons();
            lock = true;
        }
        if (pendingSpellButton_ >= 0) {
            const bool turnOn = !static_cast<SwitchButton*>(spellButtons_->getButton(pendingSpellButton_))->isOn();
            signalSpellButton.dispatch(pendingSpellButton_);
            lock = true;
            if (turnOn) toggleSpellButton(pendingSpellButton_);
        }
        if (pendingUltimate_ >= 0) {
            signalUltimate.dispatch(pendingUltimate_);
            lock = true;
        }
    }
    pendingMenuButton_ = -1;
    pendingSpellButton_ = -1;
    pendingUltimate_ = -1;
    return lock;
}

void GamePanel::toggleSpellButton(int pressedId) {
    for (int i = 0; i < spellButtons_->count(); ++i) {
        auto* button = static_cast<SwitchButton*>(spellButtons_->getButton(i));
        button->setOn(pressedId == i ? !button->isOn() : false);
    }
}

void GamePanel::releaseSpellButtons() { toggleSpellButton(-1); }

void GamePanel::createPanel() {
    const int frameColor = level_->playerRaceIndex();
    panel_ = symbols::create("GamePanelClass");
    menuButtons_ = std::make_shared<ButtonSet>();
    menuButtons_->addButton(makeButton(symbols::child(*panel_, "_-m"), BUT_MENU));
    menuButtons_->addButton(makeButton(symbols::child(*panel_, "_-oj"), BUT_FAST));
    menuButtons_->addButton(makeButton(symbols::child(*panel_, "_-cP"), BUT_FAST_ON));
    menuButtons_->addButton(makeButton(symbols::child(*panel_, "_-cX"), BUT_HELP));
    menuButtons_->signalRollOver.addListener(this, &GamePanel::onMenuOver);
    menuButtons_->signalRollOut.addListener(this, &GamePanel::onButtonOut);
    menuButtons_->signalClick.addListener(this, &GamePanel::onMenuClick);
    pendingMenuButton_ = -1;

    spellButtons_ = std::make_shared<ButtonSet>();
    for (int i = 0; i < BUT_COUNT; ++i) spellButtons_->addButton(makeSpellButton(i));
    spellButtons_->addButton(makeSpellButton(BUT_CHEAT_0));
    spellButtons_->addButton(makeSpellButton(BUT_CHEAT_1));
    spellButtons_->signalClick.addListener(this, &GamePanel::onSpellClick);
    spellButtons_->signalRollOver.addListener(this, &GamePanel::onSpellOver);
    spellButtons_->signalRollOut.addListener(this, &GamePanel::onButtonOut);
    pendingSpellButton_ = -1;

    ultimateButton_ = std::make_shared<USpellButton>(ButtonDef::make(shared(symbols::child(*panel_, "butSpell")), std::nullopt, BUT_USPELL));
    ultimateButton_->signalRollOver.addListener(this, &GamePanel::onSpellOver);
    ultimateButton_->signalRollOut.addListener(this, &GamePanel::onButtonOut);
    ultimateButton_->signalClick.addListener(this, &GamePanel::onUltimateClick);
    level_->setUltimateIcon(*ultimateButton_);
    pendingUltimate_ = -1;

    ultimateX_ = static_cast<int>(ultimateButton_->image()->x());
    centerX_ = static_cast<int>(spellButtons_->getButton(2)->image()->x());
    buttonSpacing_ = centerX_ - static_cast<int>(spellButtons_->getButton(1)->image()->x());

    if (auto* back = symbols::child(*panel_, "mcBack")) {
        for (const char* name : {"mcGoldPanel", "mcHousePanel", "mcManaPanel"})
            if (auto* part = symbols::child(*back, name)) part->gotoAndStop(frameColor);
        SquadIcon::rasterizeContents(*back);
    }
    layoutSpellButtons();
    listenResourceHints();
}

void GamePanel::destroyPanel() {
    if (panel_) unlistenResourceHints();
    if (spellButtons_) spellButtons_->dispose();
    if (menuButtons_) menuButtons_->dispose();
    if (ultimateButton_) ultimateButton_->dispose();
    spellButtons_.reset(), menuButtons_.reset(), ultimateButton_.reset();
    if (panel_ && panel_->parent() != nullptr) panel_->parent()->removeChild(panel_.get());
    panel_.reset();
    level_ = nullptr;
}

std::shared_ptr<Button> GamePanel::makeButton(engine::MovieClip* movie, int id) {
    return std::make_shared<Button>(ButtonDef::make(shared(movie), std::nullopt, id));
}

std::shared_ptr<SwitchButton> GamePanel::makeSpellButton(int id) {
    const Spell spell = spellOf(id);
    engine::MovieClip* off = symbols::child(*panel_, "butAction" + std::to_string(id));
    engine::MovieClip* on = symbols::child(*panel_, "butActionSel" + std::to_string(id));
    for (engine::MovieClip* clip : {off, on}) {
        if (auto* icons = symbols::child(*clip, "mcIconSet")) icons->gotoAndStop(spell.id + 1);
        if (id < BUT_COUNT) {
            if (auto* key = symbols::find<engine::TextField>(*clip, "mcKey.txtKey")) key->setText(std::to_string(id + 1));
            if (auto* mana = symbols::find<engine::TextField>(*clip, "mcMana.txtMana")) mana->setText(std::to_string(spell.cost));
        }
    }
    auto button = std::make_shared<SwitchButton>(ButtonDef::make(shared(off), std::nullopt, id),
                                                 ButtonDef::make(shared(on), std::nullopt, id));
    if (id < BUT_COUNT) button->setHidden(!level_->hasSpell(id));
    else button->setHidden(!Level::isModeOn(Level::ST_CHEAT_MODE));
    return button;
}

void GamePanel::layoutSpellButtons() {
    std::vector<Button*> shown;
    for (int i = 0; i < spellButtons_->count(); ++i) {
        const bool available = level_->hasSpell(i);
        static_cast<SwitchButton*>(spellButtons_->getButton(i))->setHidden(!available);
        if (available) shown.push_back(spellButtons_->getButton(i));
    }
    const int shownSpells = static_cast<int>(shown.size());
    if (ultimateButton_->image()->visible()) shown.push_back(ultimateButton_.get());
    const int totalWidth = buttonSpacing_ * static_cast<int>(shown.size());
    int x = centerX_ - totalWidth / 2;
    const int y = static_cast<int>(spellButtons_->getButton(2)->image()->y());
    for (int i = 0; i < shownSpells; ++i) {
        static_cast<SwitchButton*>(shown[static_cast<std::size_t>(i)])->moveTo(x, y);
        x += buttonSpacing_;
    }
    if (ultimateButton_->image()->visible())
        ultimateButton_->image()->setX(ultimateX_ - (4 - shownSpells - 1) * buttonSpacing_ / 2);
}

void GamePanel::setUltimateVisible(bool visible) {
    ultimateButton_->setReadOnly(!visible);
    ultimateButton_->image()->setVisible(visible);
}

void GamePanel::onMenuClick(Button* button) { pendingMenuButton_ = button->id(); }
void GamePanel::onSpellClick(Button* button) { pendingSpellButton_ = button->id(); }
void GamePanel::onUltimateClick(Button* button) { pendingUltimate_ = button->id(); }

void GamePanel::onMenuOver(Button* button) {
    Level::isCursorUnderButton = true;
    std::string text;
    switch (button->id()) {
    case BUT_MENU: text = "MENU / PAUSE"; break;
    case BUT_FAST: text = "Switch to FAST SPEED"; break;
    case BUT_FAST_ON: text = "Switch to NORMAL SPEED"; break;
    default: break;
    }
    level_->hintLayer().getHintWindow(level_->hintBottomId())->setText(text);
    const engine::Point p = button->image()->localToGlobal({0, 0});
    level_->hintLayer().showHintAtRect({p.x - 18, p.y - 18, 36, 36}, level_->hintBottomId());
}

void GamePanel::onSpellOver(Button* button) {
    Level::isCursorUnderButton = true;
    switch (button->id()) {
    case 0:
    case 1:
    case 2: showSpellHint(button->id(), *level_); break;
    case BUT_USPELL: showSpellHint(3, *level_); break;
    default: return;
    }
    level_->hintLayer().showHintAtObject(button->image(), level_->hintSpellId());
}

void GamePanel::onButtonOut(Button*) {
    level_->hintLayer().hideHint();
    Level::isCursorUnderButton = false;
}

void GamePanel::swapSpeedButtons() {
    auto* fastOn = symbols::child(*panel_, "_-cP");
    auto* fast = symbols::child(*panel_, "_-oj");
    const double y = fastOn->y();
    fastOn->setY(fast->y());
    fast->setY(y);
}

Spell GamePanel::spellOf(int buttonId) const { return Spell(*level_->playerSpell(buttonId), &level_->player().bonuses); }

void GamePanel::showResources(const IResourceSource& source) {
    engine::MovieClip* panel = symbols::child(*panel_, "_-CV");
    symbols::setHtmlText(*panel, "txtGold",
                         std::to_string(source.getGold()) + "<FONT COLOR='#9D7004'/> (+" + std::to_string(source.getGoldSpeed()) + ")</FONT>");
    const bool supplyOk = source.getArmyPower() < source.getSupplyPower();
    symbols::setHtmlText(*panel, "txtUpkeep",
                         std::string(supplyOk ? "<FONT COLOR='#99FF00'/>" : "<FONT COLOR='#FF0000'/>") +
                             std::to_string(source.getArmyPower()) + "</FONT>/<FONT COLOR='#02D202'/>" +
                             std::to_string(source.getSupplyPower()) + "</FONT>");
    symbols::setHtmlText(*panel, "_-jL", "-" + std::to_string(source.getGoldForSupply()));
    symbols::setHtmlText(*panel, "txtMana",
                         std::to_string(source.getMana()) + "<FONT COLOR='#009999'/> (+" + std::to_string(source.getManaSpeed()) + ")</FONT>");
    for (int i = 0; i < BUT_COUNT; ++i) {
        Button* button = spellButtons_->getButton(i);
        button->setEnabled(source.getMana() >= spellOf(button->id()).cost);
    }
    ultimateButton_->setReadyFactor(source.getUltimateFactor());
}

// --- Resource tooltips ---------------------------------------------------------------------------------

void GamePanel::listenResourceHints() {
    engine::MovieClip* panel = symbols::child(*panel_, "_-CV");
    panel->addEventListener(engine::MouseEvent::MOUSE_OVER, this, &GamePanel::onResourceOver);
    panel->addEventListener(engine::MouseEvent::MOUSE_OUT, this, &GamePanel::onResourceOut);
    for (const char* name : {"_-uq", "_-YN", "_-Av"})
        if (auto* glow = symbols::child<engine::DisplayObject>(*panel, name)) glow->setAlpha(0);
}

void GamePanel::unlistenResourceHints() {
    engine::MovieClip* panel = symbols::child(*panel_, "_-CV");
    if (panel == nullptr) return;
    panel->removeEventListener(engine::MouseEvent::MOUSE_OVER, this, &GamePanel::onResourceOver);
    panel->removeEventListener(engine::MouseEvent::MOUSE_OUT, this, &GamePanel::onResourceOut);
    for (const char* name : {"_-uq", "_-YN", "_-Av"}) {
        auto* glow = symbols::child<engine::DisplayObject>(*panel, name);
        fadeIn(glow, false);
        fadeOut(glow, false);
    }
}

int GamePanel::hintTypeOf(engine::DisplayObject* object) const {
    engine::MovieClip* panel = symbols::child(*panel_, "_-CV");
    if (object == symbols::child<engine::DisplayObject>(*panel, "_-uq")) return HINT_GOLD;
    if (object == symbols::child<engine::DisplayObject>(*panel, "_-YN")) return HINT_SUPPLY;
    if (object == symbols::child<engine::DisplayObject>(*panel, "_-Av")) return HINT_MANA;
    return HINT_NONE;
}

void GamePanel::onResourceOver(engine::MouseEvent& event) {
    HintWindow2* window = level_->hintLayer().getHintWindow(level_->hintOverPanelId());
    auto* target = dynamic_cast<engine::DisplayObject*>(event.target());
    const Combatant& player = level_->player();
    std::string text;
    switch (hintTypeOf(target)) {
    case HINT_GOLD:
        text += "You pay with <FONT COLOR='#FFFF00'>GOLD</FONT> to purchase troops and buildings.<br><br>";
        text += "<FONT COLOR='#999999'>Base Income (per sec):</FONT>  +" + std::to_string(player.getBaseGoldSpeed()) + " gold<br>";
        text += "<FONT COLOR='#999999'>Troops Supply Modifier:</FONT>  -" + std::to_string(player.getSupplyModifierPercent()) +
                "% (-" + std::to_string(player.getGoldForSupply()) + " gold per sec)<br><br>";
        text += "<FONT COLOR='#999999'>Net Income (per sec):</FONT>  +" + std::to_string(player.getGoldSpeed()) + " gold<br>";
        break;
    case HINT_SUPPLY:
        text += "You can't purchase troops if you don't have<br>enough free <FONT COLOR='#65FF00'> SUPPLY POINTS</FONT>.<br><br>";
        text += "<FONT COLOR='#999999'>In Use:</FONT>  " + std::to_string(player.getArmyPower()) + " supply points<br>";
        text += "<FONT COLOR='#999999'>Total:</FONT>  " + std::to_string(player.getSupplyPower()) + " supply points<br><br>";
        text += "<FONT COLOR='#999999'>Troops Supply Modifier:</FONT>  -" + std::to_string(player.getSupplyModifierPercent()) +
                "% (-" + std::to_string(player.getGoldForSupply()) + " gold per sec)<br>";
        break;
    case HINT_MANA:
        text += "To cast spell you have to spend some <FONT COLOR='#00CBFF'>MANA</FONT>.<br><br>";
        text += "<FONT COLOR='#999999'>Mana generation (per sec):</FONT>  +" + std::to_string(player.getManaSpeed()) + " mana<br>";
        text += "<FONT COLOR='#999999'>Mana Limit:</FONT>  " + std::to_string(Combatant::MAX_MANA) + " mana";
        break;
    default:
        return;
    }
    window->setText(text);
    fadeOut(target, false);
    fadeIn(target, true);
    level_->hintLayer().showHintWindowAtRect(Level::hintRect(), window);
}

void GamePanel::onResourceOut(engine::MouseEvent& event) {
    auto* target = dynamic_cast<engine::DisplayObject*>(event.target());
    if (hintTypeOf(target) != HINT_NONE) {
        level_->hintLayer().hideHint();
        fadeIn(target, false);
        fadeOut(target, true);
    }
}

void GamePanel::fadeIn(engine::DisplayObject* hint, bool enable) {
    if (hint == nullptr) return;
    if (enable) {
        hint->setAlpha(std::max(0.1, hint->alpha()));
        hint->addEventListener(engine::Event::ENTER_FRAME, this, &GamePanel::onFadeIn);
    } else {
        hint->removeEventListener(engine::Event::ENTER_FRAME, this, &GamePanel::onFadeIn);
    }
}

void GamePanel::fadeOut(engine::DisplayObject* hint, bool enable) {
    if (hint == nullptr) return;
    if (enable) hint->addEventListener(engine::Event::ENTER_FRAME, this, &GamePanel::onFadeOut);
    else hint->removeEventListener(engine::Event::ENTER_FRAME, this, &GamePanel::onFadeOut);
}

void GamePanel::onFadeIn(engine::Event& event) {
    auto* clip = dynamic_cast<engine::DisplayObject*>(event.target());
    clip->setAlpha(std::min(1.0, clip->alpha() * 1.3));
    if (clip->alpha() == 1) clip->removeEventListener(engine::Event::ENTER_FRAME, this, &GamePanel::onFadeIn);
}

void GamePanel::onFadeOut(engine::Event& event) {
    auto* clip = dynamic_cast<engine::DisplayObject*>(event.target());
    clip->setAlpha(std::max(0.0, clip->alpha() / 1.3));
    if (clip->alpha() == 0) clip->removeEventListener(engine::Event::ENTER_FRAME, this, &GamePanel::onFadeOut);
}

} // namespace takeover
