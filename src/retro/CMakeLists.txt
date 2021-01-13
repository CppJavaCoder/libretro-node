file(GLOB SRC "*.cpp")
add_library(retro STATIC ${SRC})
target_compile_features(retro PUBLIC cxx_std_17)
target_link_libraries(retro
    INTERFACE common retro::retro-api
    PRIVATE common sdl fmt::fmt retro::retro-api)

if(WIN32)
	target_compile_definitions(retro PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
