cmake_minimum_required(VERSION 3.12)

project(nats_asio)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TESTS "enable tests" OFF)
option(BUILD_NATS_TOOL "build nats tool" OFF)

add_definitions(-DSPDLOG_FMT_EXTERNAL)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
        "${CMAKE_CURRENT_LIST_DIR}/include"
)

set(LINCLUDE "${CMAKE_CURRENT_LIST_DIR}/include/nats_asio")

set(NATS_ASIO_LIB_SRC
        "${LINCLUDE}/interface.hpp"
        "impl.hpp"
        "impl.cpp"
        )

list(REMOVE_ITEM CONAN_LIBS "boost_unit_test_framework")

add_library(nats_asio ${NATS_ASIO_LIB_SRC})
target_link_libraries(nats_asio ${CONAN_LIBS})

if (BUILD_NATS_TOOL)
    add_executable(nats_tool samples/nats_tool.cpp)
    target_link_libraries(nats_tool nats_asio ${CONAN_LIBS})
endif()

if (ENABLE_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    add_executable(parser_test tests/check1.cpp)
    target_link_libraries(parser_test GTest::GTest GTest::Main ${CONAN_LIBS})
    add_test(DefaultTestSuite default_test_suite)
endif ()


