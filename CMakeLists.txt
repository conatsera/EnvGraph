cmake_minimum_required(VERSION 3.14)

    project(SpectrumAnalyzer LANGUAGES C CXX VERSION 0.1)

        set(CMAKE_CXX_STANDARD 20)

            set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

                set(CMAKE_POSITION_INDEPENDENT_CODE ON)

                    add_compile_options(-Wall - Werror - fno - permissive) if (
                        ${CMAKE_CXX_COMPILER_ID} STREQUAL
                        "Clang") add_compile_options(-stdlib = libc++) add_link_options(-fuse - ld = lld - flto -
                                                                                                     L / usr / local /
                                                                                                         lib) endif()

                        if (${CMAKE_BUILD_TYPE} STREQUAL "Release") add_compile_options(-O3) endif()

                            add_compile_definitions(VK_USE_PLATFORM_XCB_KHR) add_compile_definitions(
                                USE_ONLINE_VULKAN_SHADERC)

                                find_package(Vulkan REQUIRED) find_library(fftw3 REQUIRED) find_library(
                                    FLUTTER_LIB flutter_engine PATH $ENV{FLUTTER_ENGINE} / out /
                                    host_debug_unopt REQUIRED)

                                    set(PYTHON_EXECUTABLE / usr / bin / python3) set(GLSLANG_VALIDATOR / usr / bin /
                                                                                     glslangValidator)

                                        macro(glsl_to_spirv src basename) add_custom_command(
                                            OUTPUT shaders / ${src}.h COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR} /
                                            scripts / generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR} / ${basename} /
                                            ${src} ${CMAKE_CURRENT_BINARY_DIR} / shaders /
                                            ${src}.h $ { GLSLANG_VALIDATOR } false DEPENDS ${CMAKE_SOURCE_DIR} /
                                            scripts / generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR} / ${basename} /
                                            ${src} ${GLSLANG_VALIDATOR}) endmacro()

                                            macro(assembly_to_spirv src basename) add_custom_command(
                                                OUTPUT shaders /
                                                ${src}.h COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR} / scripts /
                                                generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR} / ${basename} /
                                                ${src} ${CMAKE_CURRENT_BINARY_DIR} / shaders /
                                                ${src}.h $ { SPIRV_TOOLS_ASSEMBLER } true DEPENDS ${CMAKE_SOURCE_DIR} /
                                                scripts / generate_spirv.py ${CMAKE_CURRENT_SOURCE_DIR} / ${basename} /
                                                ${src} ${SPIRV_TOOLS_ASSEMBLER}) endmacro()

                                                glsl_to_spirv(control.vert shaders) glsl_to_spirv(control.frag shaders)
                                                    glsl_to_spirv(radio.vert shaders) glsl_to_spirv(radio.comp shaders)
                                                        glsl_to_spirv(ui.vert shaders) glsl_to_spirv(
                                                            ui.frag shaders) glsl_to_spirv(kinect_mesh.vert shaders)

                                                            include_directories(.)

                                                                add_subdirectory(extern)

                                                                    add_library(envgraph STATIC vk_utils.cpp engine
                                                                                    .cpp pipelines.h pipelines /
                                                                                control.cpp pipelines /
                                                                                radio.cpp pipelines /
                                                                                kinect_mesh.cpp shaders /
                                                                                control.vert.h shaders /
                                                                                control.frag.h shaders /
                                                                                radio.vert.h shaders /
                                                                                ui.vert.h shaders / ui.frag.h shaders /
                                                                                kinect_mesh.vert.h)

                                                                        add_subdirectory(tests)

                                                                            target_include_directories(
                                                                                envgraph PUBLIC ${
                                                                                    Vulkan_INCLUDE_DIRS} $ENV{
                                                                                    VULKAN_SDK} /
                                                                                    Third -
                                                                                Party /
                                                                                    Include $ENV{FLUTTER_ENGINE} /
                                                                                    flutter / shell / platform /
                                                                                    embedder PRIVATE ${
                                                                                        CMAKE_BINARY_DIR} /
                                                                                    shaders)

                                                                                target_link_libraries(
                                                                                    envgraph PUBLIC ${
                                                                                        Vulkan_LIBRARIES} xcb fftw3
                                                                                        LimeSuite pthread
#${FLUTTER_LIB }
                                                                                    )

                                                                                    if (${CMAKE_CXX_COMPILER_ID} STREQUAL
                                                                                        "Clang")
                                                                                        target_link_libraries(
                                                                                            envgraph PUBLIC
                                                                                                c++ c++ abi) endif()

                                                                                            set_target_properties(
                                                                                                envgraph PROPERTIES
                                                                                                    INTERPROCEDURAL_OPTIMIZATION
                                                                                                        TRUE)