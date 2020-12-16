cmake_minimum_required(VERSION 3.14)

project(SpectrumAnalyzer LANGUAGES C CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-Wall -Werror -fno-permissive -stdlib=libc++)

add_compile_definitions(VK_USE_PLATFORM_XCB_KHR)
add_compile_definitions(USE_ONLINE_VULKAN_SHADERC)

find_package(Vulkan REQUIRED)

add_executable(graph main.cpp engine.cpp vk_utils.cpp pipelines.h pipelines/control.cpp)

target_include_directories(graph
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Third-Party/Include
)

target_link_libraries(graph
    PUBLIC
        ${Vulkan_LIBRARIES}
        c++
        c++abi
        xcb
)