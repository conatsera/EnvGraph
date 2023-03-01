cmake_minimum_required (VERSION 3.25)

project ("EnvGraph" VERSION 0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
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

add_library(engine)
target_sources(engine
    PUBLIC
      FILE_SET public_modules TYPE CXX_MODULES FILES
        interfaces/engine/pubsub.ixx
		interfaces/engine/asset.ixx
        interfaces/engine/engine.ixx
        interfaces/engine/log.ixx
        interfaces/engine/pipeline.ixx
        interfaces/engine/platform.ixx
        interfaces/engine/platforms/dx12.ixx
        interfaces/engine/platforms/windows.ixx
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
file(COPY "$ENV{VCToolsInstallDir}/modules/std.compat.ixx" "$ENV{VCToolsInstallDir}/modules/std.ixx" DESTINATION ${CMAKE_BINARY_DIR})
add_library(std_lib)
target_sources(std_lib
    PUBLIC
      FILE_SET public_modules TYPE CXX_MODULES FILES
		${CMAKE_BINARY_DIR}/std.ixx
		${CMAKE_BINARY_DIR}/std.compat.ixx
)
endif()

target_link_libraries(engine PRIVATE std_lib)

find_package(OpenSSL)
if (NOT ${OPENSSL_FOUND})
    set(OPENSSL_ROOT_DIR ./extern/openssl/install)
    find_package(OpenSSL)
    if (NOT ${OPENSSL_FOUND})
		add_custom_target(openssl-build DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/openssl/install/lib/libcrypto.lib)
        add_dependencies(engine openssl-build)
		set(OPENSSL_INCLUDE_DIR ./extern/openssl/install/include)
    endif()
endif()

file(GLOB SCHEMA_FILES LIST_DIRECTORIES FALSE asset_schemas/*.fbs)

set(SCHEMA_OUTPUT_FILES ${SCHEMA_FILES})

list(TRANSFORM SCHEMA_OUTPUT_FILES REPLACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
list(TRANSFORM SCHEMA_OUTPUT_FILES REPLACE "\.fbs$" "_generated.h")

add_custom_command(
    OUTPUT
        ${SCHEMA_OUTPUT_FILES}
    COMMAND ${CMAKE_BINARY_DIR}/extern/flatbuffers-build/flatc.exe
    ARGS    --cpp --gen-mutable --gen-object-api --reflect-names
			--cpp-ptr-type std::unique_ptr
			-o ${CMAKE_BINARY_DIR}/asset_schemas
			${SCHEMA_FILES}
    VERBATIM
	DEPENDS flatc ${SCHEMA_FILES}
)

add_custom_target(asset-schema-build DEPENDS ${SCHEMA_OUTPUT_FILES})
add_dependencies(engine asset-schema-build libvips-build)

target_include_directories(engine
	PRIVATE
		${OPENSSL_INCLUDE_DIR}
		${CMAKE_BINARY_DIR}/asset_schemas
		${CMAKE_SOURCE_DIR}/extern/flatbuffers/include
		${LIBVIPS_INCLUDE_DIRS}
)

target_link_directories(engine PUBLIC ${LIBVIPS_DEV_ROOT}/lib)
target_link_libraries(engine
	PRIVATE
		OpenSSL::Crypto OpenSSL::SSL
		flatbuffers
		${LIBVIPS_LIBRARIES}
		${LIBVIPS_DEV_ROOT}/lib/libintl.a
		${LIBVIPS_DEV_ROOT}/lib/libvips-cpp.dll.a
)

if(CUFFT_FFT_BACKEND)
    target_include_directories(engine PUBLIC "$ENV{CUDA_PATH}/include")
endif()

if(ONEAPI_FFT_BACKEND)
endif()


#add_subdirectory(src)
add_subdirectory(tests)