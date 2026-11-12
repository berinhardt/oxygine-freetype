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
if (ANDROID)
   # Android.mk reached the framework headers through
   # '$(LOCAL_PATH)/../oxygine-framework/oxygine/src/', listed in
   # LOCAL_C_INCLUDES. ndk-build therefore recorded that spelling in __FILE__
   # for the framework headers this module includes, relative to the module's
   # own directory (e.g.
   # '../../deps/oxygine-freetype/../oxygine-framework/oxygine/src/oxygine/...').
   # The inherited include directory is the collapsed absolute one, which the
   # root prefix map rewrites to '../../deps/oxygine-framework/oxygine/src/...'.
   # The literal lives in this module's own translation units, so a PRIVATE
   # directory is enough; BEFORE puts it ahead of the inherited collapsed
   # directory, so clang opens the header through the spelling ndk-build used.
   # The include directory alone is sufficient: no matching prefix map is
   # needed.
   target_include_directories(oxygine-freetype BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/../oxygine-framework/oxygine/src")
   # The rest of LOCAL_C_INCLUDES: the glm and SDL spellings ndk-build used.
   target_include_directories(oxygine-freetype BEFORE PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/../glm/"
      "${CMAKE_CURRENT_SOURCE_DIR}/../SDL/include/")
endif()

