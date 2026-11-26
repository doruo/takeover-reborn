// ProfileIcon.h - one save slot on the profile screen, with its campaign
// progress and the delete confirmation (original: _SafePkg_3473/ProfileIcon).
#pragma once

#include "engine/display/MovieClip.h"
#include "game/framework/ui/Button.h"

#include <memory>

namespace takeover {

    class GameStorage;
    class Profile;

    class ProfileIcon : public Disposable {
    public:
        ProfileIcon(engine::MovieClip* slot, GameStorage& storage, int id);

        Signal<ProfileIcon*> signalClick;

        /// (icon, confirmed) after the delete question (original: _SafeStr_3526).
        Signal<ProfileIcon*, bool> signalDelete;

        [[nodiscard]] Profile& profile() const;
        [[nodiscard]] bool isEmpty() const;
        [[nodiscard]] int id() const { return id_; }

        void recreateSlot(bool enable = true);

    protected:
        void customDispose() override;

    private:
        void onIconClick(Button*);          ///< _SafeStr_3654
        void showCompanyProgress(engine::MovieClip* icon, int companyId); ///< _SafeStr_3652
        void onDeleteClick(Button*);        ///< _SafeStr_3655 / _SafeStr_3656
        void disposeConfirmButtons();       ///< _SafeStr_3651
        void onDeleteNo(Button*);           ///< _SafeStr_3657
        void onDeleteYes(Button*);          ///< _SafeStr_3653

        std::shared_ptr<engine::MovieClip> slot_;
        GameStorage* storage_;
        int id_;
        std::shared_ptr<Button> iconButton_, deleteButton_, yesButton_, noButton_;
    };

} // namespace takeover
