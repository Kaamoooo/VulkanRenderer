cmake_minimum_required(VERSION 3.25)
project(VulkanTestClion)

set(CMAKE_CXX_STANDARD 17)

add_custom_target(
        CompileShaders
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Shaders/compileShaders.bat
        COMMENT "Compiling Shader Files"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
        VERBATIM
        )

include_directories("E:\\Vulkan\\glfw-3.3.8.bin.WIN64\\include")
include_directories("E:\\Vulkan\\SDK\\Include")

add_executable(VulkanTestClion Source/main.cpp Headers/MyWindow.hpp Headers/Application.hpp Headers/Pipeline.hpp Source/Device.cpp Headers/Device.hpp Source/Application.cpp Source/MyWindow.cpp Source/Pipeline.cpp Source/SwapChain.cpp Headers/Model.hpp Source/Model.cpp Source/Model.cpp Headers/GameObject.h Headers/Renderer.h Source/Renderer.cpp Headers/RenderSystem.h Source/RenderSystem.cpp Headers/Camera.h Headers/Camera.h Source/Camera.cpp Headers/KeyboardMovementController.h Source/KeyboardMovementController.cpp)

target_link_libraries(${PROJECT_NAME} "E:\\Vulkan\\SDK\\Lib\\vulkan-1.lib")
target_link_libraries(${PROJECT_NAME} "E:\\Vulkan\\glfw-3.3.8.bin.WIN64\\lib-mingw-w64\\libglfw3.a")

