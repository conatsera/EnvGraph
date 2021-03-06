include(GoogleTest)

add_executable(edge-ut depth/edge_ut.cc)

target_link_libraries(edge-ut PRIVATE envgraph gtest)

target_include_directories(edge-ut PRIVATE "${gtest_SOURCE_DIR}/include")

add_executable(view-ut ui/view_ut.cc)

target_link_libraries(view-ut PRIVATE envgraph gtest)

target_include_directories(view-ut PRIVATE "${gtest_SOURCE_DIR}/include")