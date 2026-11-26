add_executable(assetpack main.cpp BitmapDecoder.cpp)
target_link_libraries(assetpack PRIVATE takeover::swf third_party::stb third_party::json ZLIB::ZLIB takeover::warnings)
