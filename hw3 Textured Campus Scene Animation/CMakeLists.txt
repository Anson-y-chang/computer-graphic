cmake_minimum_required(VERSION 3.12.0)
project(hello_window VERSION 1.0.0)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 20)

# 🔧 關掉不必要的 GLFW 功能，只編譯靜態庫
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Do not build GLFW docs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Do not build GLFW tests" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Do not build GLFW examples" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build GLFW as static lib" FORCE)

# 🔧 加入 GLFW 原始碼
add_subdirectory(dependencies/glfw)

# OpenGL
find_package(OpenGL REQUIRED)

# 🔧 建立執行檔
add_executable(hello_window
    src/main.cpp 
    dependencies/glad/glad.c 
    src/utils/callbacks.cpp
    src/utils/utils.cpp
    src/utils/camera_path.cpp
)

# 包含標頭檔
target_include_directories(hello_window 
    PRIVATE
    dependencies
)

# 🔧 連結 GLFW + OpenGL
target_link_libraries(hello_window
    PRIVATE
    glfw
    OpenGL::GL
)

add_custom_command(TARGET hello_window POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/models
    ${CMAKE_BINARY_DIR}/models
)