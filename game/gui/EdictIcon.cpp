#include "game/gui/EdictIcon.h"

#include "game/framework/Symbols.h"
#include "game/gui/GameScreen.h"

namespace takeover {

    EdictIcon::EdictIcon(Edict& edict, engine::MovieClip* icon, int iconFrame)
        : edict_(&edict), icon_(sharedClip(icon)), iconFrame_(iconFrame),
          button_(std::make_shared<Button>(ButtonDef::make(icon_, std::nullopt))) {
        button_->signalClick.addListener(this, &EdictIcon::onClick);
    }

    void EdictIcon::customDispose() {
        if (button_)
            button_->dispose();

        button_.reset();
        signalClick.dispose();
        Disposable::customDispose();
    }

    void EdictIcon::updateIconView(int edictCount) {
        symbols::setHtmlText(*icon_, "mcCaption.txtCaption", edict_->name);
        const bool canBuy = edictCount >= edict_->getRequiredPoints();
        std::string hint;

        for (int i = 0; i < 3; ++i) {
            const bool enabled = i < edict_->curPoints;
            const bool nextEnabled = i + 1 < edict_->curPoints;
            const bool present = i < static_cast<int>(edict_->getEffects().size());
            std::string prefix;

            if (enabled)
                prefix = i == 0 || !nextEnabled ? "<FONT COLOR='#FFFF00'>" : "<FONT COLOR='#B99187'>";
            else if (canBuy)
                prefix = "<FONT COLOR='#B99187'>";
            else
                prefix = "<FONT COLOR='#B666666'>";

            if (present)
                hint = edict_->getEffects()[static_cast<std::size_t>(i)].hint;

            showLevel(icon_.get(), enabled, canBuy, i, present, hint, prefix);
        }

        symbols::child(*icon_, "mcIcon")->gotoAndStop(iconFrame_);
        button_->setEnabled(canBuy);
        engine::MovieClip* lock = symbols::child(*icon_, "mcLockScroll");
        lock->setVisible(!canBuy);
        symbols::setHtmlText(*lock, "txtNum", std::to_string(edict_->getRequiredPoints()));
        lock->gotoAndStop(1);
    }

    void EdictIcon::showLevel(engine::MovieClip* icon, bool enabled, bool canBuy, int id, bool present,
                              const std::string& hint, const std::string& prefix) {
        engine::MovieClip* scroll = symbols::child(*icon, "mcScrollIcon" + std::to_string(id));
        scroll->gotoAndStop(enabled ? 1 : (canBuy ? 2 : 3));
        symbols::setHtmlText(*scroll, "txtNum", prefix + std::to_string(id + 1) + "</FONT>");
        scroll->setVisible(present);

        engine::MovieClip* stripe = symbols::child(*icon, "mcStripe" + std::to_string(id));

        if (id == 0) {
            std::string caption;

            if (!enabled)
                caption += canBuy ? "<FONT COLOR='#B99188'>" : "<FONT COLOR='#B666666'>";

            caption += "UNLOCKS";
            symbols::setHtmlText(*stripe, "mcCaption", caption);
        } else {
            symbols::setHtmlText(*stripe, "mcCaption", prefix + hint + "</FONT>");
        }

        if (enabled)
            stripe->gotoAndStop("_up");
        else
            stripe->gotoAndStop(canBuy ? "_over" : "_disabled");
    }

    void EdictIcon::onClick(Button*) { signalClick.dispatch(this); }

} // namespace takeover
