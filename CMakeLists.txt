cmake_minimum_required (VERSION 3.16)
project (OXYGINE_FREETYPE)

set(OXYGINE_FREETYPE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(oxygine-freetype STATIC 
	src/ResFontFT.cpp 
	src/ResFontFT.h
)
target_link_libraries(oxygine-freetype PUBLIC oxygine-framework)
target_include_directories(oxygine-freetype PUBLIC ${OXYGINE_FREETYPE_INCLUDE_DIRS})

