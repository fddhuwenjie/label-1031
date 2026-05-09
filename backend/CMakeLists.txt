cmake_minimum_required(VERSION 3.16)
project(WaterSimulation VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 平台检测
message(STATUS "Building for: ${CMAKE_SYSTEM_NAME} (${CMAKE_SYSTEM_PROCESSOR})")

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX-)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# Find packages
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

# GLAD source
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/third_party/glad)

# 检查 GLAD 是否存在
if(NOT EXISTS "${GLAD_DIR}/src/glad.c")
    message(FATAL_ERROR 
        "GLAD not found at ${GLAD_DIR}\n"
        "Please generate GLAD files first:\n"
        "  pip3 install glad\n"
        "  python3 -m glad --generator=c --out-path=third_party/glad --api=\"gl=3.3\" --profile=core"
    )
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GLAD_DIR}/include
)

# Source files
set(SOURCES
    src/main.cpp
    src/shader.cpp
    src/water_simulation.cpp
    src/camera.cpp
    src/renderer.cpp
    ${GLAD_DIR}/src/glad.c
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES})

# 平台特定链接
if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        glfw
        glm::glm
        OpenGL::GL
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME}
        glfw
        glm::glm
        OpenGL::GL
        ${CMAKE_DL_LIBS}
        pthread
    )
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME}
        glfw
        glm::glm
        OpenGL::GL
    )
endif()

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/shaders DESTINATION share/${PROJECT_NAME})
