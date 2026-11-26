add_executable(swfdump main.cpp)
target_link_libraries(swfdump PRIVATE takeover::swf takeover::warnings)
