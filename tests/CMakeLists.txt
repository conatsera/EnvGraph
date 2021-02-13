include(GoogleTest)

add_executable(edge-ut depth/edge_ut.cc)

target_link_libraries(edge-ut PRIVATE envgraph gtest)

target_include_directories(edge-ut PRIVATE "${gtest_SOURCE_DIR}/include")