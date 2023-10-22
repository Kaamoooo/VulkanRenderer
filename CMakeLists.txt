cmake_minimum_required(VERSION 3.25)
project(VulkanTestClion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories("E:\\Vulkan\\glfw-3.3.8.bin.WIN64\\include")
include_directories("E:\\Vulkan\\SDK\\Include")

file(GLOB_RECURSE SOURCES 
        ${PROJECT_SOURCE_DIR}/Source/*.cpp
        ${PROJECT_SOURCE_DIR}/Source/Systems/*.cpp
        )
add_executable(${PROJECT_NAME} ${SOURCES} Source/Sampler.h)
#target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/Source)
#add_executable(VulkanTestClion Source/main.cpp Source/MyWindow.hpp Source/Application.hpp Source/Pipeline.hpp Source/Device.cpp Source/Device.hpp Source/Application.cpp Source/MyWindow.cpp Source/Pipeline.cpp Source/SwapChain.cpp Source/Model.hpp Source/Model.cpp Source/Model.cpp Source/GameObject.h Source/Renderer.h Source/Renderer.cpp Source/RenderSystem.h Source/RenderSystem.cpp Source/Camera.h Source/Camera.h Source/Camera.cpp Source/KeyboardMovementController.h Source/KeyboardController.cpp Source/GameObject.cpp Source/Untils.h Source/Buffer.h Source/Buffer.cpp Source/FrameInfo.h Source/Descriptor.h Source/Descriptor.cpp)

target_link_libraries(${PROJECT_NAME} "E:\\Vulkan\\SDK\\Lib\\vulkan-1.lib")
target_link_libraries(${PROJECT_NAME} "E:\\Vulkan\\glfw-3.3.8.bin.WIN64\\lib-mingw-w64\\libglfw3.a")

#Build Shaders
find_program(GLSL_VALIDATOR glslangValidator HINTS
        ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
        /usr/bin
        /usr/local/bin
        ${VULKAN_SDK_PATH}/Bin
        ${VULKAN_SDK_PATH}/Bin32
        $ENV{VULKAN_SDK}/Bin/
        $ENV{VULKAN_SDK}/Bin32/
        )
