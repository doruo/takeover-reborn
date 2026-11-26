add_executable(shaperender main.cpp)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
target_link_libraries(shaperender PRIVATE takeover::swf takeover::vector PkgConfig::CAIRO third_party::stb takeover::warnings)
