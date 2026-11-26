Take Over - C++ port of the Flash strategy game (IriySoft, 2011)

Start the game:      ./takeover.sh
Fullscreen:          ./takeover.sh --fullscreen   (or F11 / Alt+Enter in game)
Bigger window:       ./takeover.sh --scale 2

Saves go to ~/.local/share/takeover (TAKEOVER_SAVE_DIR overrides it).

Needs a Linux desktop with OpenGL 3.3, and the libraries cairo, zlib and
SDL3 (SDL3 is bundled in lib/ when the build had to compile it).
