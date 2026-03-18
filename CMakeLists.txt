cmake_minimum_required(VERSION 3.20)
project(simple3dgame LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output executable to root bin/ for easy running.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)

find_package(OpenGL REQUIRED)

# Project headers (learnopengl, glm, stb_image, etc.)
include_directories(${CMAKE_SOURCE_DIR}/includes)

# Pull GLAD/GLFW sources so the project can build on either MSVC or MinGW.
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    learnopengl_glad
    GIT_REPOSITORY https://github.com/JoeyDeVries/LearnOpenGL.git
    GIT_TAG master
)

FetchContent_MakeAvailable(glfw)

FetchContent_GetProperties(learnopengl_glad)
if(NOT learnopengl_glad_POPULATED)
    FetchContent_Populate(learnopengl_glad)
endif()

add_library(glad STATIC
    ${learnopengl_glad_SOURCE_DIR}/src/glad.c
)
target_include_directories(glad PUBLIC
    ${learnopengl_glad_SOURCE_DIR}/includes
)

add_executable(coin_runner
    src/7.coin_runner/coin_runner.cpp
)

target_link_libraries(coin_runner
    OpenGL::GL
    glad
    glfw
)

if(WIN32)
    target_link_libraries(coin_runner
        user32
        gdi32
        shell32
    )
endif()

add_custom_command(TARGET coin_runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/src/7.coin_runner/7.coin_runner.vs
    $<TARGET_FILE_DIR:coin_runner>/7.coin_runner.vs
)
add_custom_command(TARGET coin_runner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/src/7.coin_runner/7.coin_runner.fs
    $<TARGET_FILE_DIR:coin_runner>/7.coin_runner.fs
)
