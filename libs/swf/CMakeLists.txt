# swf: parser for the SWF container format (dictionary, timelines, linkage).
# Used by the offline asset tools only; the game itself never reads SWF files.
add_library(swf
    src/Movie.cpp
    src/Records.cpp
    src/Shape.cpp
    src/DrawList.cpp
    src/TagCode.cpp
)
add_library(takeover::swf ALIAS swf)
target_include_directories(swf PUBLIC include)
target_link_libraries(swf PUBLIC ZLIB::ZLIB PRIVATE takeover::warnings)
