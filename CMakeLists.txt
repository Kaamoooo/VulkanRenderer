cmake_minimum_required(VERSION 3.25)
project(VulkanTestClion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories("E:\\Vulkan\\glfw-3.3.8.bin.WIN64\\include")
include_directories("E:\\Vulkan\\SDK\\Include")
include_directories("External")

file(GLOB_RECURSE SOURCES 
        ${PROJECT_SOURCE_DIR}/Source/*.cpp
        ${PROJECT_SOURCE_DIR}/Source/RenderSystems/*.cpp
        )
add_executable(${PROJECT_NAME} ${SOURCES} Source/RenderSystems/ShadowSystem.h Source/RenderSystems/GrassSystem.h Source/InputController.cpp Source/Utils/JsonUtils.hpp Source/Components/Component.hpp Source/Components/Component.hpp Source/Components/TransformComponent.hpp Source/Components/Component.hpp Source/Utils/Utils.hpp Source/Utils/Utils.hpp Source/Components/MeshRendererComponent.hpp Source/Components/LightComponent.hpp Source/Components/CameraComponent.hpp Source/RenderSystems/SkyBoxSystem.hpp)

target_link_libraries(${PROJECT_NAME} "E:\\Vulkan\\SDK\\Lib\\vulkan-1.lib")
target_link_libraries(${PROJECT_NAME} "E:\\Vulkan\\glfw-3.3.8.bin.WIN64\\lib-mingw-w64\\libglfw3.a")
