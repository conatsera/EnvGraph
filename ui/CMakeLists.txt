add_library(ui STATIC
    view_controller.cc
    view.cc
)

target_include_directories(ui
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Third-Party/Include
)