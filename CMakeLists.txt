cmake_minimum_required (VERSION 3.25)

project ("EnvGraph" VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "3c375311-a3c9-4396-a187-3227ef642046")

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(TESTING_ENABLED "Enable tests" On)

option(CUFFT_FFT_BACKEND "Enable CuFFT backend" On)
option(ONEAPI_FFT_BACKEND "Enable oneAPI backend" On)

if (WIN32)
	option(DIRECTX_GPU_PLATFORM "Enable DirectX 12 GPU platform" On)
	option(VULKAN_GPU_PLATFORM "Enable Vulkan GPU platform" On)

	add_compile_definitions(UNICODE)
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
	else()
		set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDebug)
	endif()
elseif(UNIX)
	option(DIRECTX_GPU_PLATFORM "Enable DirectX 12 GPU platform" Off)
	option(VULKAN_GPU_PLATFORM "Enable Vulkan GPU platform" On)
else()
	message("Platform ${CMAKE_HOST_SYSTEM_NAME} not supported.")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	string(CONCAT CMAKE_EXPERIMENTAL_CXX_SCANDEP_SOURCE
	  "${CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS}"
	  " -format=p1689 --p1689-targeted-file-name=<SOURCE> --p1689-targeted-output=<OBJECT> "
	  " --p1689-makeformat-output=<DEP_FILE>"
	  " --"
	  " <DEFINES> <INCLUDES> <FLAGS> -x c++ <SOURCE>"
	  " -MT <DYNDEP_FILE> -MD"
	  " > <DYNDEP_FILE>")
	set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "clang")
	set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG "@<MODULE_MAP_FILE>")

	# Default to C++ extensions being off. Clang's modules support have trouble
	# with extensions right now.
	set(CMAKE_CXX_EXTENSIONS OFF)

	if (UNIX)
		add_link_options(-fuse-ld=lld -flto -L/usr/local/lib)
	endif()

	if (CMAKE_CXX_COMPILER MATCHES ".*cl\.exe$")
        add_compile_options(/W3 /permissive-)
	else()
		add_compile_options(-Wall -Werror -fno-permissive)
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/W4 /permissive-)
endif()

add_subdirectory(extern)

#add_subdirectory(interfaces)
add_library(engine)
target_sources(engine
    PUBLIC
      FILE_SET public_modules TYPE CXX_MODULES FILES
        interfaces/engine/pubsub.ixx
        interfaces/engine/engine.ixx
        interfaces/engine/log.ixx
        interfaces/engine/pipeline.ixx
        interfaces/engine/platform.ixx
        interfaces/engine/platforms/dx12.ixx
        interfaces/engine/platforms/windows.ixx
)

if(CUFFT_FFT_BACKEND)
    target_include_directories(engine PUBLIC "$ENV{CUDA_PATH}/include")
endif()

if(ONEAPI_FFT_BACKEND)
endif()


#add_subdirectory(src)
add_subdirectory(tests)