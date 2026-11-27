# engine: a small Flash-like runtime (display list, timelines, rendering,
# audio, input) on SDL3 + OpenGL 3.3.
add_library(engine
    assets/Library.cpp
    audio/Sound.cpp
    events/EventDispatcher.cpp
    display/BitmapData.cpp
    display/DisplayObject.cpp
    display/DisplayObjectContainer.cpp
    display/Graphics.cpp
    display/MovieClip.cpp
    display/Shape.cpp
    display/SimpleButton.cpp
    display/Stage.cpp
    display/TextField.cpp
    platform/Application.cpp
    platform/Window.cpp
    render/Gl.cpp
    render/Renderer.cpp
    render/ShapeCache.cpp
    render/Texture.cpp
)
add_library(takeover::engine ALIAS engine)
# Includes are written "engine/display/MovieClip.h".
target_include_directories(engine PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(engine
    PUBLIC takeover::swf takeover::vector SDL3::SDL3 third_party::stb
    PRIVATE third_party::json third_party::miniaudio takeover::warnings)
