set(UI_PLATFORM_SRC "")
if(WIN32)
    list(APPEND UI_PLATFORM_SRC "view_win32.cc")
endif()

add_library(ui STATIC
    view_controller.cc
    view.cc
    input.cc
    ${UI_PLATFORM_SRC}
)

target_include_directories(ui
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        $ENV{VULKAN_SDK}/Third-Party/Include
)
target_compile_definitions(ui
    PUBLIC
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

target_link_directories(ui PUBLIC "$ENV{VS170COMNTOOLS}..\\..\\VC\\Tools\\MSVC\\14.30.30705\\atlmfc\\lib\\spectre\\x64")

target_link_libraries(ui PUBLIC ninput Shcore)