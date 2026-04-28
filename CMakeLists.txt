cmake_minimum_required(VERSION 3.24)
project(ZoneDrifterPrototype LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(zone_drifter
    src/main.cpp
    src/Chunk.cpp
    src/ChunkManager.cpp
    src/DeltaLayer.cpp
    src/Game.cpp
    src/Hash.cpp
    src/Object.cpp
    src/Player.cpp
    src/Types.cpp
    src/WorldGenerator.cpp
)

target_include_directories(zone_drifter PRIVATE src)
target_link_libraries(zone_drifter PRIVATE raylib)

add_executable(determinism_check
    tools/determinism_check.cpp
    src/Chunk.cpp
    src/Hash.cpp
    src/Object.cpp
    src/Types.cpp
    src/WorldGenerator.cpp
)

target_include_directories(determinism_check PRIVATE src)
