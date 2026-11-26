# The game: a port of the original's AS3 and Haxe code on top of the engine.
add_library(takeover_game STATIC
    # framework (the original's in-house AS3 library)
    framework/Symbols.cpp
    framework/audio/SoundManager.cpp
    framework/core/As3.cpp
    framework/core/Disposable.cpp
    framework/core/Format.cpp
    framework/core/Trash.cpp
    framework/input/KeyboardDevice.cpp
    framework/raster/AnimationMaker.cpp
    framework/raster/RasterAnimation.cpp
    framework/raster/RasterLayer.cpp
    framework/raster/RasterObject.cpp
    framework/raster/ResourceManager.cpp
    resources/TOResManager.cpp
    resources/XMLData.cpp
    framework/input/MouseDevice.cpp
    framework/states/StateManager.cpp
    framework/storage/ByteArray.cpp
    framework/storage/SaveFile.cpp
    framework/storage/SerializableObject.cpp
    framework/tween/Tween.cpp
    framework/ui/Button.cpp
    framework/ui/HintWindow.cpp
    framework/ui/MovieClipButton.cpp
    # simulation (the original's Haxe code)
    sim/base/BaseObject.cpp
    sim/base/Flags.cpp
    sim/base/Point2D.cpp
    sim/base/TypedKey.cpp
    sim/battle/ArmyStat.cpp
    sim/battle/Combatant.cpp
    sim/battle/Edicts.cpp
    sim/battle/MissionStats.cpp
    sim/battle/CombatantBattle.cpp
    sim/battle/Race.cpp
    sim/abilities/Abilities.cpp
    sim/abilities/AbilityEffects.cpp
    sim/abilities/Morale.cpp
    sim/abilities/Property.cpp
    sim/abilities/UnitParams.cpp
    sim/fsm/StateMachine.cpp
    sim/level/KeyNode.cpp
    sim/level/LevelAi.cpp
    sim/level/LevelLogic.cpp
    sim/level/NodeContent.cpp
    sim/level/RoadGraph.cpp
    sim/spells/SpellInfo.cpp
    sim/squads/Squad.cpp
    sim/squads/SquadBase.cpp
    sim/units/MovingBody.cpp
    sim/units/Unit.cpp
    sim/units/UnitBody.cpp
    sim/units/UnitCombat.cpp
    # campaign data and progress
    data/CompanyInfo.cpp
    data/LevelInfo.cpp
    progress/Badges.cpp
    progress/CompanyProgress.cpp
    progress/Conditions.cpp
    progress/LevelProgress.cpp
    progress/Profile.cpp
    GameStorage.cpp
    # presentation
    media/MediaToggle.cpp
    media/MusicConst.cpp
    media/SoundConst.cpp
    media/SoundConstBattle.cpp
    gui/BadgesRoomScreen.cpp
    gui/BattleScreen.cpp
    gui/Branding.cpp
    gui/CampaignScreens.cpp
    gui/ChooseProfileScreen.cpp
    gui/EdictIcon.cpp
    gui/GameScreen.cpp
    gui/IntroScreen.cpp
    gui/MediaSwitchers.cpp
    gui/MissionOverScreen.cpp
    gui/PlayerStatPanel.cpp
    gui/PreloaderScreen.cpp
    gui/ProfileIcon.cpp
    gui/ThroneRoomScreen.cpp
    gui/TitleScreen.cpp
    hud/GamePanel.cpp
    hud/WindowBase.cpp
    hud/WindowController.cpp
    level/Level.cpp
    level/Effects.cpp
    level/LevelLand.cpp
    level/LevelParser.cpp
    level/LevelVisual.cpp
    level/NodeViews.cpp
    units/FUnit.cpp
    squads/SquadIcon.cpp
    squads/FSquad.cpp
    GameBattle.cpp
    Main.cpp
)
add_library(takeover::game ALIAS takeover_game)
target_include_directories(takeover_game PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(takeover_game PUBLIC takeover::engine ZLIB::ZLIB PRIVATE takeover::warnings)

add_executable(takeover main.cpp)
target_link_libraries(takeover PRIVATE takeover::game takeover::warnings)
# Exported symbols make the crash handler's call stacks readable.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(takeover PRIVATE -rdynamic)
endif()

# --- Installation: bin/takeover, the asset pack and a launcher -----------------
# The game finds its assets in ../share/takeover/assets next to the binary.
include(GNUInstallDirs)
set_target_properties(takeover PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
install(TARGETS takeover RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
# The asset pack is generated from the original SWF (see README): installed
# when present.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/takeover/assets OPTIONAL)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/packaging/takeover.sh DESTINATION .)
install(FILES ${PROJECT_SOURCE_DIR}/packaging/README.txt DESTINATION .)
# SDL3 built from source (no system package) travels with the game.
if(TARGET SDL3-shared)
    install(TARGETS SDL3-shared LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
