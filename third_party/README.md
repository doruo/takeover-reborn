# Vendored third-party code

Single-header libraries are copied here verbatim (pinned versions, licenses
alongside) so the project builds offline and reproducibly. Do not edit these
files; update them by copying a newer release.

| Directory | Library | Version | License |
|---|---|---|---|
| `stb/` | stb_image, stb_image_write | stb_image 2.34 | MIT / public domain |
| `miniaudio/` | miniaudio | 0.11.22 | MIT-0 / public domain |
| `nlohmann/` | JSON for Modern C++ | 3.11.3 | MIT |
| `doctest/` | doctest | 2.4.11 | MIT |

SDL3 is not vendored: CMake uses the system package (`libsdl3-dev`) when
available and otherwise downloads the pinned release (see `cmake/Dependencies.cmake`).
