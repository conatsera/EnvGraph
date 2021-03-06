cmake_minimum_required(VERSION 3.14)

project(SpectrumAnalyzer LANGUAGES C CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GCC" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND NOT ${CMAKE_CXX_COMPILER} MATCHES ".*cl\.exe$")
    add_compile_options(-Wall -Werror -fno-permissive)
    if (WIN32)
        add_compile_options(-Wno-error=defaulted-function-deleted -Wno-error=deprecated-volatile)
    endif()
else()
    add_compile_options(/W3 /permissive-)
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND NOT WIN32)
    add_compile_options(-stdlib=libc++)
    add_link_options(-fuse-ld=lld -flto -L/usr/local/lib)
endif()

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_compile_options(-O3)
endif()

if (UNIX)
add_compile_definitions(VK_USE_PLATFORM_XCB_KHR)
elseif (WIN32)
add_compile_definitions(UNICODE)
add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR)
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
elseif (APPLE)

endif()
add_compile_definitions(USE_ONLINE_VULKAN_SHADERC)

find_package(Vulkan REQUIRED)
find_library(fftw3_LIB fftw3 HINTS ENV fftw3_ROOT)
find_library(LimeSuite_LIB LimeSuite HINTS ENV LimeSuite_ROOT PATH_SUFFIXES lib)
#find_library(FLUTTER_LIB flutter_engine PATH $ENV{FLUTTER_ENGINE}/out/host_debug_unopt REQUIRED)

if (UNIX)
    set(PYTHON_EXECUTABLE /usr/bin/python3)
    set(GLSLANG_VALIDATOR /usr/bin/glslangValidator)
elseif (WIN32)
    find_file(PYTHON_EXECUTABLE python.exe HINTS "C:/Program Files (x86)/Microsoft Visual Studio/Shared/Python37_64")
    find_file(GLSLANG_VALIDATOR glslangValidator.exe REQUIRED)
endif()

macro(glsl_to_spirv src basename)
    add_custom_command(OUTPUT shaders/${src}.h
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${CMAKE_CURRENT_BINARY_DIR}/shaders/${src}.h ${GLSLANG_VALIDATOR} false
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${GLSLANG_VALIDATOR}
        )
endmacro()

macro(assembly_to_spirv src basename)
    add_custom_command(OUTPUT shaders/${src}.h
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${CMAKE_CURRENT_BINARY_DIR}/shaders/${src}.h ${SPIRV_TOOLS_ASSEMBLER} true
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR}/${basename}/${src} ${SPIRV_TOOLS_ASSEMBLER}
        )
endmacro()

glsl_to_spirv(control.vert shaders)
glsl_to_spirv(control.frag shaders)
glsl_to_spirv(radio.vert shaders)
glsl_to_spirv(radio.comp shaders)
glsl_to_spirv(ui.vert shaders)
glsl_to_spirv(ui.frag shaders)
glsl_to_spirv(kinect_mesh.vert shaders)

include_directories(.)

add_subdirectory(extern)

add_subdirectory(ui)

add_library(envgraph STATIC
    vk_utils.cpp engine.cpp pipelines.h
    pipelines/control.cpp pipelines/radio.cpp
    pipelines/kinect_mesh.cpp shaders/control.vert.h
    shaders/control.frag.h shaders/radio.vert.h
    shaders/ui.vert.h shaders/ui.frag.h
    shaders/kinect_mesh.vert.h
)

target_link_libraries(envgraph
    PUBLIC
        ui
)

add_subdirectory(tests)

target_include_directories(envgraph
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Third-Party/Include
        $ENV{FLUTTER_ENGINE}/flutter/shell/platform/embedder
        $ENV{fftw3_ROOT}/../api
        $ENV{LimeSuite_ROOT}/include
    PRIVATE
        ${CMAKE_BINARY_DIR}/shaders
)

target_link_libraries(envgraph
PUBLIC
    ${Vulkan_LIBRARIES}
    ${fftw3_LIB}
    ${LimeSuite_LIB}
 #   ${FLUTTER_LIB}       
)

if (UNIX)
    target_link_libraries(envgraph
        PUBLIC
            xcb
            pthread
    )
endif()

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND NOT WIN32)
target_link_libraries(envgraph
    PUBLIC
        c++
        c++abi
)
endif()

set_target_properties(envgraph PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)