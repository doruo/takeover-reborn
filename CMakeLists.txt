cmake_minimum_required(VERSION 3.24)

project(TakeoverReborn
    VERSION 1.1.5 # version number of the original Flash release
    DESCRIPTION "C++ port of the Flash strategy game Take Over"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd / IDE tooling

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(TAKEOVER_BUILD_TOOLS "Build the SWF inspection / asset conversion tools" ON)
option(TAKEOVER_BUILD_TESTS "Build unit tests" ON)
option(TAKEOVER_BUILD_GAME "Build the game executable (needs SDL3 and OpenGL)" ON)

# One interface target carries the warning policy so every target opts in the
# same way (target_link_libraries(x PRIVATE takeover::warnings)).
add_library(takeover_warnings INTERFACE)
add_library(takeover::warnings ALIAS takeover_warnings)
if(MSVC)
    target_compile_options(takeover_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(takeover_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        -Wno-sign-conversion)
endif()

find_package(ZLIB REQUIRED)

add_subdirectory(third_party)
add_subdirectory(libs/swf)
add_subdirectory(libs/vector)

if(TAKEOVER_BUILD_GAME)
    include(cmake/Dependencies.cmake)
    add_subdirectory(engine)
    add_subdirectory(game)
    if(TAKEOVER_BUILD_TOOLS)
        add_subdirectory(tools/clipview)
    endif()
endif()

if(TAKEOVER_BUILD_TOOLS)
    add_subdirectory(tools/swfdump)
    add_subdirectory(tools/shaperender)
    add_subdirectory(tools/assetpack)
endif()

# --- Release archive: cpack -G TGZ (see tools/package.sh) ----------------------
set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_NAME takeover)
set(CPACK_PACKAGE_VENDOR "Iriysoft, ArmorGames, Doruo")
set(CPACK_PACKAGE_FILE_NAME "takeover-${PROJECT_VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_STRIP_FILES ON)
include(CPack)
