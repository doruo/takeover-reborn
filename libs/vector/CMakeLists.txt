# vector: CPU rasterization of SWF draw lists (cairo backend).
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

add_library(vector src/Rasterizer.cpp)
add_library(takeover::vector ALIAS vector)
target_include_directories(vector PUBLIC include)
target_link_libraries(vector PUBLIC takeover::swf PRIVATE PkgConfig::CAIRO takeover::warnings)
