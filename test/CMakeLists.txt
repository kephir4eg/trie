cmake_minimum_required(VERSION 2.8.0)

# file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
#file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)

set(TEST_SRCS triefunc.cpp)

foreach(testSrc ${TEST_SRCS})
    get_filename_component(testName ${testSrc} NAME_WE)
    add_executable(${testName} ${testSrc})
    target_link_libraries(${testName} ${Boost_LIBRARIES} machine)
    add_test(NAME ${testName} COMMAND ${testName})
endforeach(testSrc)
