// EdictIcon.h - one edict on the throne room screen (original:
// _SafePkg_3473/EdictIcon, built from the definition _SafeStr_3463).
#pragma once

#include "game/framework/ui/Button.h"
#include "game/sim/battle/Edicts.h"

namespace takeover {

    class EdictIcon : public Disposable {
    public:
        EdictIcon(Edict& edict, engine::MovieClip* icon, int iconFrame);

        Signal<EdictIcon*> signalClick; ///< Original: _SafeStr_3469.
        [[nodiscard]] Edict& edict() const { return *edict_; }

        /// Shows the levels bought, buyable or locked. `edictCount` is the
        /// number of points spent in the campaign.
        void updateIconView(int edictCount);

    protected:
        void customDispose() override;

    private:
        void showLevel(engine::MovieClip* icon, bool enabled, bool canBuy, int id, bool present, const std::string& hint,
                       const std::string& prefix); ///< _SafeStr_3623
        void onClick(Button*);

        Edict* edict_;
        std::shared_ptr<engine::MovieClip> icon_;
        int iconFrame_;
        std::shared_ptr<Button> button_;
    };

} // namespace takeover
