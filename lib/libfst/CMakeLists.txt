cmake_minimum_required (VERSION 3.0)

project (fstlib)

########################################################################################################################
# Easiest way to get ZLib on windows is using vcpkg, to load vcpkg:
# cmake -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
if(CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using VCPKG from ${CMAKE_TOOLCHAIN_FILE}")
endif()
########################################################################################################################
find_package(ZLIB REQUIRED)

add_library(fstapi fstapi.c fstapi.h fst_win_unistd.h)
target_link_libraries(fstapi PRIVATE ZLIB::ZLIB)
# hack to avoid creating dummy config.h
target_compile_definitions(fstapi PRIVATE -DFST_CONFIG_INCLUDE="fstapi.h")

if(MSVC)
    # define __MINGW32__ to minimize changes to upstream
    target_compile_definitions(fstapi PRIVATE __MINGW32__  _CRT_SECURE_NO_WARNINGS  FST_DO_MISALIGNED_OPS)
    target_compile_options(fstapi PRIVATE /wd4244 /wd4267 /wd4146 /wd4996)
endif()
