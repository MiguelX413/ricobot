# SPDX-License-Identifier: AGPL-3.0-only
cmake_minimum_required(VERSION 3.20)
project(RicoBotNG
    VERSION 0.1.0
    DESCRIPTION "RicoBot Next Generation, a Metamod bot for Half-Life: Ricochet"
    LANGUAGES CXX
)

add_library(ricobot_mm MODULE
    bot.cpp
    bot_client.cpp
    bot_config.cpp
    bot_lifecycle.cpp
    dll.cpp
    engine.cpp
    h_export.cpp
    util.cpp
)
set_target_properties(ricobot_mm PROPERTIES
    PREFIX ""
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_include_directories(ricobot_mm PRIVATE
    hlsdk/common
    hlsdk/dlls
    hlsdk/engine
    hlsdk/pm_shared
    metamod
)
target_compile_definitions(ricobot_mm PRIVATE
    RICOBOT_NG_VERSION="${PROJECT_VERSION}"
    HAVE_STDINT_H
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(ricobot_mm PRIVATE
        _LINUX POSIX LINUX linux stricmp=strcasecmp strcmpi=strcasecmp
    )
    target_compile_options(ricobot_mm PRIVATE
        -m32 -msse2 -mfpmath=sse -fno-strict-aliasing
        -fno-exceptions -fno-rtti -Wno-attributes -Wno-write-strings
    )
    set_target_properties(ricobot_mm PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )
    target_link_options(ricobot_mm PRIVATE -m32 -Wl,--no-undefined)
    target_link_libraries(ricobot_mm PRIVATE ${CMAKE_DL_LIBS} m)
elseif(WIN32 AND MSVC)
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "RicoBot NG requires x86. Configure Visual Studio with -A Win32.")
    endif()
    target_compile_definitions(ricobot_mm PRIVATE
        WIN32 _WINDOWS NOMINMAX _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
    )
    target_compile_options(ricobot_mm PRIVATE /arch:SSE2 /EHsc)
    target_link_options(ricobot_mm PRIVATE
        "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8,@1"
        "/SECTION:.data,RW"
        /MACHINE:X86
    )
    set_target_properties(ricobot_mm PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
else()
    message(FATAL_ERROR "RicoBot NG's CMake build supports Linux and Windows with MSVC.")
endif()
