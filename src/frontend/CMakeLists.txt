file(GLOB SRC "*.cpp" "**/**.cpp" "**/**.c")
add_library(frontend STATIC ${SRC})
target_compile_features(frontend PRIVATE cxx_std_17)

find_package(OpenGL REQUIRED)

target_link_libraries(frontend
    PRIVATE OpenGL::GL sdl2::sdl2 sdl2::sdl2-ttf sdl2::sdl2-image glew::glew
        fmt::fmt nano-signal-slot::nano-signal-slot stb utfcpp::utfcpp retro::retro-api
    INTERFACE nano-signal-slot::nano-signal-slot stb sdl2::sdl2)

if(WIN32)
	target_compile_definitions(frontend PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()
