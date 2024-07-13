cmake_minimum_required(VERSION 3.25)
project(VulkanTestClion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories("E:\\Vulkan\\glfw-3.3.8.bin.WIN64\\include")
include_directories("E:\\Vulkan\\SDK\\Include")

include_directories("External")

file(GLOB_RECURSE SOURCES
        ${PROJECT_SOURCE_DIR}/Source/*.cpp
        ${PROJECT_SOURCE_DIR}/Source/*.hpp
        ${PROJECT_SOURCE_DIR}/External/Imgui/*.cpp
        )
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} "E:\\Vulkan\\SDK\\Lib\\vulkan-1.lib")
target_link_libraries(${PROJECT_NAME} "E:\\Vulkan\\glfw-3.3.8.bin.WIN64\\lib-mingw-w64\\libglfw3.a")
