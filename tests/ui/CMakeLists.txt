add_executable(view-ut view_ut.cc)

target_link_libraries(view-ut PRIVATE engine ui gtest)

target_include_directories(view-ut PRIVATE "${gtest_SOURCE_DIR}/include")

add_executable(input-ut input_ut.cc)

target_link_libraries(input-ut PRIVATE engine ui gtest)

target_include_directories(input-ut PRIVATE "${gtest_SOURCE_DIR}/include")