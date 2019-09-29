cmake_minimum_required(VERSION 3.12)

project(nats-asio)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_TESTS "enable tests" ON)

add_definitions(-DSPDLOG_FMT_EXTERNAL)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DSPDLOG_FMT_EXTERNAL)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Boost 1.71 COMPONENTS coroutine REQUIRED)
find_package(FMT REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
	"${CMAKE_CURRENT_LIST_DIR}/include"
	"${CMAKE_CURRENT_LIST_DIR}/src"
	"${CMAKE_INSTALL_PREFIX}/include"
	"${CMAKE_PREFIX_PATH}/include"
	"${Boost_INCLUDE_DIRS}"
	"${OPENSSL_INCLUDE_DIR}"
	)

set(LINCLUDE "${CMAKE_CURRENT_LIST_DIR}/include/nats-asio" )

set(nats_asio_SRCS
	"${LINCLUDE}/defs.hpp"
	"${LINCLUDE}/common.hpp"
	"${LINCLUDE}/fwd.hpp"
	"${LINCLUDE}/interface.hpp"
	"src/connection.hpp"
	"src/structs.hpp"
	"src/parser.hpp"
	"src/common.cpp"
	"src/connection.cpp"
	"src/parser.cpp"
)


add_library(nats_asio ${nats_asio_SRCS})
add_executable(nats_asio_check main.cpp)

target_link_libraries(nats_asio_check  pthread nats_asio "${Boost_COROUTINE_LIBRARIES}" OpenSSL::Crypto OpenSSL::SSL fmt::fmt)

if (ENABLE_TESTS)
	enable_testing()
	find_package(GTest REQUIRED)
	get_filename_component(GTEST_LIB_FOLDER ${GTEST_LIBRARY} DIRECTORY)
	include_directories(${GTEST_INCLUDE_DIRS})
	link_directories(${GTEST_LIB_FOLDER})
	add_executable(parser_test check1.cpp)
	target_link_libraries(parser_test GTest::GTest GTest::Main gmock gmock_main nats_asio "${Boost_COROUTINE_LIBRARIES}" OpenSSL::Crypto OpenSSL::SSL fmt::fmt)
	add_test(DefaultTestSuite default_test_suite)
endif()


