cmake_minimum_required(VERSION 3.20)
project(GridSnake VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    message(FATAL_ERROR "Use GCC 9+ (MinGW-w64 12+ recommended on Windows), or Visual Studio 2022. Older Windows libstdc++ has incomplete C++17 filesystem support.")
endif()

option(SNAKE_BUILD_GAME "Build the raylib desktop game" ON)
include(CTest)

function(snake_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(snake_executable target)
    snake_warnings(${target})
    if(MINGW)
        # The Windows executable can run without MinGW's DLLs on PATH.
        target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endfunction()

add_library(snake_core STATIC src/Game.cpp src/Board.cpp)
target_include_directories(snake_core PUBLIC src)
snake_warnings(snake_core)

add_library(snake_storage STATIC src/ScoreStorage.cpp)
target_include_directories(snake_storage PUBLIC src)
snake_warnings(snake_storage)

if(SNAKE_BUILD_GAME)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Disable raylib examples" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "Disable raylib games" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build raylib statically" FORCE)
    set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "Use the static MSVC runtime" FORCE)
    if(POLICY CMP0135)
        cmake_policy(SET CMP0135 NEW)
    endif()
    FetchContent_Declare(raylib
        URL https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5
        URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    )
    FetchContent_MakeAvailable(raylib)

    add_executable(grid_snake src/main.cpp src/Renderer.cpp)
    target_link_libraries(grid_snake PRIVATE snake_core snake_storage raylib)
    snake_executable(grid_snake)
endif()

if(BUILD_TESTING)
    add_executable(snake_tests tests/GameTests.cpp)
    target_link_libraries(snake_tests PRIVATE snake_core)
    snake_executable(snake_tests)
    foreach(case IN ITEMS
        initial_state normal_movement eating reverse_and_repeat rapid_turns
        planned_direction walls self_collision departing_tail food_placement
        deterministic_seed victory restart pause title_transition fixed_timestep
        invalid_time difficulty)
        add_test(NAME logic.${case} COMMAND snake_tests ${case})
    endforeach()

    add_executable(score_storage_tests tests/ScoreStorageTests.cpp)
    target_link_libraries(score_storage_tests PRIVATE snake_storage)
    snake_executable(score_storage_tests)
    add_test(NAME storage.roundtrip_and_errors COMMAND score_storage_tests)
endif()
