#add_subdirectory(graphics)
#add_subdirectory(compute)
add_subdirectory(ui)

add_subdirectory(vk)
#add_subdirectory(dx12)

#add_subdirectory(examples)

add_library(engine STATIC engine.cc common.cc)

if (VK_SUBSYSTEM_ENABLED)
    target_link_libraries(engine vk-subsystem)
endif()
