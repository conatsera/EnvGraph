enable_testing()

set(TESTS "log" "engine" "pubsub" "asset")
set(TEST_SOURCES "")

foreach(TEST IN LISTS TESTS)
	list(APPEND TEST_SOURCES "${TEST}-ut.cc")

	add_executable("test-${TEST}" "${TEST}-ut.cc")
	target_link_libraries("test-${TEST}" PRIVATE gtest_main engine)
	target_include_directories("test-${TEST}" PUBLIC "../extern/googletest/googletest/include")
endforeach()

add_executable("test-all" ${TEST_SOURCES})
target_link_libraries("test-all" PRIVATE gtest_main engine)
target_include_directories("test-all" PUBLIC "../extern/googletest/googletest/include")

file(COPY ./1_webp_ll.png ./1_webp_ll.webp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})