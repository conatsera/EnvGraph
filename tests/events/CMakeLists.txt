add_executable(event-ut event_ut.cc "mock_object.h")

target_link_libraries(event-ut PRIVATE engine ui gtest ${SDL2_LIBS})

target_include_directories(event-ut PRIVATE "${gtest_SOURCE_DIR}/include")
