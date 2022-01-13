add_executable(utils-ut depth/utils_ut.cc)

target_link_libraries(utils-ut PRIVATE dxlib gtest)

target_include_directories(utils-ut
	PRIVATE
		"${gtest_SOURCE_DIR}/include"
		"$ENV{VULKAN_SDK}/Third-Party/Include"
)

add_executable(view-ut ui/view_ut.cc)

target_link_libraries(view-ut PRIVATE dxlib gtest)

target_include_directories(view-ut PRIVATE "${gtest_SOURCE_DIR}/include")