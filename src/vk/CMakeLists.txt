add_library(vk-subsystem STATIC subsystem.cc)

target_link_libraries(vk-subsystem
	PUBLIC
		${Vulkan_LIBRARIES})
