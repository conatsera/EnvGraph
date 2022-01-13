add_executable(engine-ut engine_ut.cc)

target_link_libraries(engine-ut PRIVATE engine gtest ui ${SDL2_LIBS})

target_include_directories(engine-ut PRIVATE "${gtest_INCLUDE_DIRS}")

add_executable(pipelines-ut pipelines_ut.cc)

target_link_libraries(pipelines-ut PRIVATE engine gtest ui ${SDL2_LIBS})

target_include_directories(pipelines-ut PRIVATE "${gtest_INCLUDE_DIRS}")

add_executable(log-ut log_ut.cc)

target_link_libraries(log-ut PRIVATE gtest)

target_include_directories(log-ut PRIVATE "${gtest_INCLUDE_DIRS}")