cmake_minimum_required (VERSION 3.16)

set(OXYGINE_FREETYPE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Android keeps clang's default C++ dialect; the Emscripten and native graphs
# pin C++11.
if (NOT ANDROID)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(oxygine-freetype STATIC 
	src/ResFontFT.cpp 
	src/ResFontFT.h
)
target_link_libraries(oxygine-freetype PUBLIC oxygine-framework)
target_include_directories(oxygine-freetype PUBLIC ${OXYGINE_FREETYPE_INCLUDE_DIRS})
if (ANDROID)
   # This module's own translation units reach the framework headers through
   # this `..` spelling, so a PRIVATE directory is enough; BEFORE puts it ahead
   # of the inherited collapsed directory. Changing the spelling changes the
   # path clang records for the header (`__FILE__` and debug info), not the
   # code.
   target_include_directories(oxygine-freetype BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/../oxygine-framework/oxygine/src")
   # The glm and SDL include directories.
   target_include_directories(oxygine-freetype BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/../glm/"
      "${CMAKE_CURRENT_SOURCE_DIR}/../SDL/include/")
endif()

