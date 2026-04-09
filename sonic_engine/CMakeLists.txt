cmake_minimum_required(VERSION 3.16)
project(SonicEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find SFML 3.0
find_package(SFML 3 COMPONENTS Graphics Window System QUIET)

# Fallback to SFML 2.x if 3.0 not found
if(NOT SFML_FOUND)
    find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
    message(STATUS "Using SFML 2.x")
else()
    message(STATUS "Using SFML 3.x")
endif()

# Engine library sources
set(ENGINE_SOURCES
    Engine/src/Bitmap.cpp
    Engine/src/Input.cpp
    Engine/src/SystemClock.cpp
    Engine/src/Game.cpp
    Engine/src/TileLayer.cpp
    Engine/src/GridLayer.cpp
    Engine/src/Animator.cpp
    Engine/src/AnimationFilm.cpp
)

# Create engine library
add_library(SonicEngineLib STATIC ${ENGINE_SOURCES})
target_include_directories(SonicEngineLib PUBLIC Engine/include)
target_link_libraries(SonicEngineLib PUBLIC sfml-graphics sfml-window sfml-system)

# Create executable
add_executable(SonicGame 
    App/src/main.cpp
    App/src/SonicPlayer.cpp
    App/src/TunnelSystem.cpp
)
target_include_directories(SonicGame PRIVATE App/include)
target_link_libraries(SonicGame PRIVATE SonicEngineLib)

# Output directories
set_target_properties(SonicGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(SonicEngineLib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Copy assets to build directory
file(COPY Assets DESTINATION ${CMAKE_BINARY_DIR})
