if (${TESTING_ENABLED})
    add_subdirectory(googletest EXCLUDE_FROM_ALL)
endif()

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/openssl/install/lib/libcrypto.lib
    COMMAND perl ARGS Configure VC-WIN64A 
    --strict-warnings --api=3.0 no-deprecated enable-fips threads
    no-gost no-legacy no-ocb no-idea no-bf no-cast
    no-rc2 no-rc4 no-rc5 no-ripemd
    no-seed no-sm2 no-sm3 no-sm4 /wd4267 /wd4244 /wd4311 /wd4133
    "--prefix=C:/Users/austin/source/repos/conatsera/EnvGraph/extern/openssl/install"
    "--openssldir=C:/Users/austin/source/repos/conatsera/EnvGraph/extern/openssl/install_ssl"
    COMMAND nmake
    COMMAND nmake ARGS test
    COMMAND nmake ARGS install
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/openssl
    VERBATIM
)
set(PREV_CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)
add_subdirectory(
    flatbuffers
    ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers-build
    EXCLUDE_FROM_ALL
)
set(CMAKE_BUILD_TYPE ${PREV_CMAKE_BUILD_TYPE})

set(LIBVIPS_VERSION 8.14 CACHE STRING "Version of libvips to build. n.n" )

if (WIN32)
set(LIBVIPS_BUILD_WIN64_MXE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build-win64-mxe CACHE STRING "Root directory of MXE/Docker-based build system")

set(LIBVIPS_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/vips-dev-${LIBVIPS_VERSION}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vips-dev-${LIBVIPS_VERSION}/include/glib-2.0"
    "${CMAKE_CURRENT_SOURCE_DIR}/vips-dev-${LIBVIPS_VERSION}/lib/glib-2.0/include"
    
    CACHE STRING "built libvips include directory")

find_program(DOCKER_EXECUTABLE docker.exe)
find_program(WSL_EXECUTABLE wsl.exe)

execute_process(COMMAND wsl -- wslpath ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE WSL_CURRENT_SOURCE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

find_file(LIBVIPS_DEV_ROOT vips-dev-${LIBVIPS_VERSION} PATHS ${CMAKE_CURRENT_SOURCE_DIR})

if (${LIBVIPS_DEV_ROOT} STREQUAL "LIBVIPS_DEV_ROOT-NOTFOUND")
    set(LIBVIPS_DEV_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/vips-dev-${LIBVIPS_VERSION} CACHE STRING "Root directory of built libvips")
endif()

set(LIBVIPS_LIBRARY ${LIBVIPS_DEV_ROOT}/lib/libvips.lib CACHE STRING "build libvips library")
if (WIN32)
    set(LIBVIPS_CPP_LIBRARY libvips-cpp-msvc CACHE STRING "build libvips-cpp library")
else()
    set(LIBVIPS_CPP_LIBRARY ${LIBVIPS_DEV_ROOT}/lib/libvips-cpp.lib CACHE STRING "build libvips-cpp library")
endif()

find_file(CHECK_FOR_LIBVIPS libvips.lib PATH ${LIBVIPS_DEV_ROOT}/lib/)

if (${CHECK_FOR_LIBVIPS} STREQUAL "CHECK_FOR_LIBVIPS-NOTFOUND")
    add_custom_command(
        OUTPUT  ${LIBVIPS_LIBRARY} ${LIBVIPS_CPP_LIBRARY}
        COMMAND ${WSL_EXECUTABLE} ARGS "--" "cp" "${WSL_CURRENT_SOURCE_DIR}/libvips-win-build.sh" "/tmp"
        COMMAND ${WSL_EXECUTABLE} ARGS "--cd" "/tmp" "--" "./libvips-win-build.sh"
        COMMAND ${WSL_EXECUTABLE} ARGS "--cd" "/tmp/build-win64-mxe" "--" "./build.sh" "--with-hevc" "all"
        COMMAND ${WSL_EXECUTABLE} ARGS "--" "cp" "-r" "/tmp/build-win64-mxe/build/vips-dev-${LIBVIPS_VERSION}" "${WSL_CURRENT_SOURCE_DIR}"
        DEPENDS ${WSL_EXECUTABLE} ${DOCKER_EXECUTABLE}
        VERBATIM
    )
endif()

if (WIN32)
    find_library(LIBVIPS_CPP_MSVC libvips-cpp-msvc PATH ${LIBVIPS_DEV_ROOT}/lib)
    if (${LIBVIPS_CPP_MSVC} STREQUAL "LIBVIPS_CPP_MSVC-NOTFOUND")
        add_library(libvips-cpp-msvc
            libvips/cplusplus/VImage.cpp
            libvips/cplusplus/VInterpolate.cpp
            libvips/cplusplus/VConnection.cpp
            libvips/cplusplus/VError.cpp
            libvips/cplusplus/VRegion.cpp
        )

        target_compile_definitions(libvips-cpp-msvc PUBLIC VIPS_CPLUSPLUS_EXPORTS)
        
        target_include_directories(libvips-cpp-msvc PRIVATE ${LIBVIPS_INCLUDE_DIRS})
        target_link_directories(libvips-cpp-msvc PRIVATE ${LIBVIPS_DEV_ROOT}/lib/)
        target_link_libraries(libvips-cpp-msvc PUBLIC ${LIBVIPS_LIBRARY} libglib-2.0 libgobject-2.0)
    endif()
endif()

set(LIBVIPS_LIBRARIES
    ${LIBVIPS_LIBRARY}
    ${LIBVIPS_CPP_LIBRARY}
    CACHE STRING "build libvips library"
)

add_custom_target(libvips-build DEPENDS ${LIBVIPS_LIBRARY} ${LIBVIPS_CPP_LIBRARY})

elseif(UNIX)

else()
    
endif()