cmake_minimum_required(VERSION 3.12)

project(nats_asio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_TESTS "enable tests" OFF)
option(BUILD_NATS_TOOL "build nats tool" OFF)

add_definitions(-DSPDLOG_FMT_EXTERNAL)

#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(cxxopts REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(
        "${CMAKE_CURRENT_LIST_DIR}/include"
        ${Boost_INCLUDE_DIRS}
        ${spdlog_INCLUDE_DIRS}
        ${OpenSSL_INCLUDE_DIR}
        ${cxxopts_INCLUDE_DIRS}
        ${fmt_INCLUDE_DIRS}
        ${nlohmann_json_INCLUDE_DIRS}
)

set(LINCLUDE "${CMAKE_CURRENT_LIST_DIR}/include/nats_asio")

set(NATS_ASIO_LIB_SRC
        "${LINCLUDE}/interface.hpp"
        "impl.hpp"
        "impl.cpp"
        )

add_library(nats_asio ${NATS_ASIO_LIB_SRC})
target_link_libraries(nats_asio
    boost::boost
    spdlog::spdlog_header_only
    fmt::fmt
    nlohmann_json::nlohmann_json
    openssl::openssl
)

if (BUILD_NATS_TOOL)
    add_executable(nats_tool samples/nats_tool.cpp)
    target_link_libraries(nats_tool nats_asio
        boost::boost
        spdlog::spdlog_header_only
        fmt::fmt
        nlohmann_json::nlohmann_json
        openssl::openssl
    )
endif()

if (ENABLE_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    add_executable(parser_test tests/check1.cpp)
    target_link_libraries(parser_test GTest::GTest GTest::Main)
    add_test(DefaultTestSuite default_test_suite)
endif ()


