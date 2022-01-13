cmake_minimum_required(VERSION 3.15)

project("EnvGraph" VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(TESTING_ENABLED "Enable tests" On)

add_subdirectory(extern)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GCC" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (NOT CMAKE_CXX_COMPILER MATCHES ".*cl\.exe$")
        add_compile_options(-Wall -Werror -fno-permissive)
        if (WIN32)
            add_compile_options(-Wno-error=defaulted-function-deleted -Wno-error=deprecated-volatile -Wno-defaulted-function-deleted)
        endif()
    else()
        add_compile_options(/W3 /permissive-)
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (WIN32)
        add_compile_options(-Wno-error=defaulted-function-deleted -Wno-error=deprecated-volatile -Wno-defaulted-function-deleted)
    else()
        #add_compile_options(-stdlib=libc++)
        add_link_options(-fuse-ld=lld -flto -L/usr/local/lib)
    endif()
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
endif()

set(VK_SUBSYSTEM_ENABLED 1)
add_compile_definitions(VK_SUBSYSTEM_ENABLED)

if (UNIX)
    if (VK_SUBSYSTEM_ENABLED)
        add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
    endif()
elseif (WIN32)
    add_compile_definitions(UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

    if (VK_SUBSYSTEM_ENABLED)
        add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
    endif()
elseif (APPLE)

endif()

if (VK_SUBSYSTEM_ENABLED)
    find_package(Vulkan REQUIRED)
endif()

find_file(GLM_HPP_FOUND glm.hpp REQUIRED HINTS "$ENV{VULKAN_SDK}/Third-Party/Include" PATH_SUFFIXES glm)

set(ENVGRAPH_INCLUDE_DIRS
        "${CMAKE_SOURCE_DIR}/include"
        "${Vulkan_INCLUDE_DIRS}"
        "$ENV{VULKAN_SDK}/Third-Party/Include" CACHE PATH "EnvGraph_INCLUDE_DIRS")

include_directories(
    ${ENVGRAPH_INCLUDE_DIRS}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(src)

enable_testing()

add_library(EnvGraph STATIC include/engine.h "include/graphics/objects.h" "include/ui/input.h")
target_link_libraries(EnvGraph PUBLIC engine ui)

if (${TESTING_ENABLED})
    add_subdirectory(tests)
endif()

#install(TARGETS eg eglib)