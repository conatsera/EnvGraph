include(GoogleTest)

add_compile_definitions(TEST_DEFAULT_WIDTH=512 TEST_DEFAULT_HEIGHT=512)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/test/bin")

add_subdirectory(events)

add_subdirectory(ui)

add_subdirectory(engine)

add_subdirectory(depth)

if (WIN32)

	find_file(SDL_DLL SDL2.dll HINTS "$ENV{VULKAN_SDK}/Third-Party/Bin")

	file(COPY ${SDL_DLL} DESTINATION "${PROJECT_BINARY_DIR}/test/bin")

endif()