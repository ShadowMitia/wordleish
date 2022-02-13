cmake_minimum_required(VERSION 3.4)

project(wordle
    VERSION 1.0.0
    LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)

### Load YACHT ###
include(${PROJECT_SOURCE_DIR}/cmake/yacht.cmake) # will containe most default flags
add_compile_commands_json()

### CONAN PACKAGE MANAGER ###
conan_get_package(PACKAGE fmt/8.0.1 imgui/1.86 sdl/2.0.18)

add_library(glad "${PROJECT_SOURCE_DIR}/external/glad/src/gl.c")
target_include_directories(glad SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/external/glad/include/" "${PROJECT_SOURCE_DIR}/external")

### SOURCE & INCLUDES ###

add_executable(wordle "src/main.cpp" "${PROJECT_SOURCE_DIR}/external/imgui_impl_sdl.cpp" "${PROJECT_SOURCE_DIR}/external/imgui_impl_opengl3.cpp")

target_link_libraries(wordle PUBLIC CONAN_PKG::sdl CONAN_PKG::fmt CONAN_PKG::imgui)
target_include_directories(wordle SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/external")

# Asset copying
add_custom_target(copy_assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_dependencies(wordle copy_assets)
