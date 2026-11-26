# Vendored single-header libraries, each exposed as a CMake target.
# Their own warnings are not our concern: they are compiled without
# takeover::warnings and included as SYSTEM headers.

add_library(stb STATIC stb_impl.c)
target_include_directories(stb SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_library(third_party::stb ALIAS stb)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_library(third_party::json ALIAS nlohmann_json)

add_library(doctest INTERFACE)
target_include_directories(doctest SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_library(third_party::doctest ALIAS doctest)

if(TAKEOVER_BUILD_GAME)
    add_library(miniaudio STATIC miniaudio_impl.c)
    target_include_directories(miniaudio SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    find_package(Threads REQUIRED)
    target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS} m)
    add_library(third_party::miniaudio ALIAS miniaudio)
endif()
