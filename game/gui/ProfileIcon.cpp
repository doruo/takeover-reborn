#include "game/gui/ProfileIcon.h"

#include "game/GameStorage.h"
#include "game/framework/Symbols.h"
#include "game/framework/core/Trash.h"
#include "game/gui/GameScreen.h"

namespace takeover {

    ProfileIcon::ProfileIcon(engine::MovieClip* slot, GameStorage& storage, int id)
        : slot_(sharedClip(slot)), storage_(&storage), id_(id) {
        recreateSlot();
    }

    Profile& ProfileIcon::profile() const { return storage_->profile(id_); }
    bool ProfileIcon::isEmpty() const { return !profile().enabled(); }

    void ProfileIcon::customDispose() {
        if (iconButton_)
            iconButton_->dispose();

        if (deleteButton_)
            deleteButton_->dispose();

        iconButton_.reset();
        deleteButton_.reset();
        disposeConfirmButtons();
        signalClick.removeAllListeners();
        signalDelete.removeAllListeners();
        slot_.reset();
        storage_ = nullptr;
        Disposable::customDispose();
    }

    void ProfileIcon::recreateSlot(bool enable) {
        if (iconButton_) {
            iconButton_->dispose();
            iconButton_.reset();
        }

        engine::MovieClip* icon = nullptr;

        if (!isEmpty() && enable) {
            slot_->gotoAndStop("lbSlotNotEmpty");
            icon = symbols::child(*slot_, "mcIconNotEmpty");
            symbols::child<engine::TextField>(*icon, "txtSlot")->setText("SLOT " + std::to_string(id_ + 1));
            showCompanyProgress(symbols::child(*icon, "mcProgressWestaria"), CompanyInfo::WESTARIA);
            showCompanyProgress(symbols::child(*icon, "mcProgressKhaganat"), CompanyInfo::KHAGANATE);
            showCompanyProgress(symbols::child(*icon, "mcProgressCult"), CompanyInfo::CULT);

            if (deleteButton_)
                deleteButton_->dispose();

            deleteButton_ = std::make_shared<Button>(ButtonDef::make(sharedClip(symbols::child(*slot_, "butDelete")), std::nullopt));
            deleteButton_->signalClick.addListener(this, &ProfileIcon::onDeleteClick);
        } else {
            slot_->gotoAndStop("lbSlotEmpty");
            icon = symbols::child(*slot_, "mcIconEmpty");
        }

        iconButton_ = std::make_shared<Button>(ButtonDef::make(sharedClip(icon), std::nullopt));
        iconButton_->setEnabled(enable);
        iconButton_->signalClick.addListener(this, &ProfileIcon::onIconClick);
    }

    void ProfileIcon::onIconClick(Button*) { signalClick.dispatch(this); }

    void ProfileIcon::showCompanyProgress(engine::MovieClip* icon, int companyId) {
        const CompanyProgress& campaign = profile().company(companyId);
        symbols::setHtmlText(*icon, "mcRank.txtField", std::to_string(campaign.rank()));
        symbols::setHtmlText(*icon, "mcProgress.txtField",
                             std::to_string(campaign.completedCount()) + "/" + std::to_string(campaign.levelCount()));
    }

    void ProfileIcon::onDeleteClick(Button*) {
        slot_->gotoAndStop("lbConfirmDelete");
        disposeConfirmButtons();
        yesButton_ = std::make_shared<Button>(ButtonDef::make(sharedClip(symbols::find(*slot_, "mcIconDelete.butYes")), std::nullopt));
        yesButton_->signalClick.addListener(this, &ProfileIcon::onDeleteYes);
        noButton_ = std::make_shared<Button>(ButtonDef::make(sharedClip(symbols::find(*slot_, "mcIconDelete.butNo")), std::nullopt));
        noButton_->signalClick.addListener(this, &ProfileIcon::onDeleteNo);
    }

    void ProfileIcon::disposeConfirmButtons() {
        // The confirm buttons may be the ones being clicked: release them later.
        for (auto* button : {&yesButton_, &noButton_}) {
            if (*button) {
                (*button)->dispose();
                releaseLater(*button);
                button->reset();
            }
        }
    }

    void ProfileIcon::onDeleteNo(Button*) {
        disposeConfirmButtons();
        signalDelete.dispatch(this, false);
        recreateSlot();
    }

    void ProfileIcon::onDeleteYes(Button*) {
        disposeConfirmButtons();
        signalDelete.dispatch(this, true);
        recreateSlot();
    }

} // namespace takeover
