// TitleScreen.h - the title screen with START, and its animated background
// with the logos (original: gui/TitleScreen and gui/TitleScreenBack).
#pragma once

#include "game/gui/Branding.h"
#include "game/gui/GameScreen.h"

namespace takeover {

/// Original: gui/TitleScreenBack.
class TitleScreenBack {
public:
    explicit TitleScreenBack(engine::MovieClip* background);
    void dispose() { branding_.reset(); }

private:
    std::unique_ptr<Branding> branding_;
};

class TitleScreen : public GameScreen {
public:
    static constexpr int BUT_START = 0;
    explicit TitleScreen(std::shared_ptr<ScreenDef> def);

protected:
    void initButtonsBlock() override;
    void disposeButtonsBlock() override;

private:
    std::unique_ptr<TitleScreenBack> background_;
};

/// The animated introduction before the title (original: gui/GameIntro).
class GameIntro : public GameScreen {
public:
    static constexpr int BUT_SKIP = 0;
    explicit GameIntro(std::shared_ptr<ScreenDef> def);
    void update(double timeElapsed) override;

protected:
    void initButtonsBlock() override;
};

/// "Campaign complete": choose another campaign or keep this one
/// (original: gui/VictoryScreen).
class VictoryScreen : public GameScreen {
public:
    static constexpr int BUT_YES = 0;
    static constexpr int BUT_NO = 1;
    explicit VictoryScreen(std::shared_ptr<GameScreenDef> def);

protected:
    void initButtonsBlock() override;
};

} // namespace takeover
