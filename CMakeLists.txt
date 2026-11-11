cmake_minimum_required (VERSION 3.16)

set(OXYGINE_FREETYPE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Android keeps clang's default C++ dialect: the ndk-build baseline compiled
# this module at that default, and a byte comparison of the two builds needs
# the same codegen. The Emscripten and native graphs keep the C++11 pin.
if (NOT ANDROID)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(oxygine-freetype STATIC 
	src/ResFontFT.cpp 
	src/ResFontFT.h
)
target_link_libraries(oxygine-freetype PUBLIC oxygine-framework)
target_include_directories(oxygine-freetype PUBLIC ${OXYGINE_FREETYPE_INCLUDE_DIRS})

