cmake_minimum_required(VERSION 3.12)

project(nats_asio)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_TESTS "enable tests" OFF)
option(ENABLE_EXAMPLES "enable build examples" OFF)

add_definitions(-DSPDLOG_FMT_EXTERNAL)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DSPDLOG_FMT_EXTERNAL)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Boost COMPONENTS coroutine REQUIRED)
find_package(FMT REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
	"${CMAKE_CURRENT_LIST_DIR}/include"
	"${CMAKE_CURRENT_LIST_DIR}/src"
	"${CMAKE_PREFIX_PATH}/include"
	"${CMAKE_INSTALL_PREFIX}/include"
	"${Boost_INCLUDE_DIRS}"
	"${OPENSSL_INCLUDE_DIR}"
	)

set(LINCLUDE "${CMAKE_CURRENT_LIST_DIR}/include/nats_asio" )

set(nats_asio_PUB_INCLUDES
	"${LINCLUDE}/defs.hpp"
	"${LINCLUDE}/common.hpp"
	"${LINCLUDE}/fwd.hpp"
	"${LINCLUDE}/interface.hpp"
	)

set(nats_asio_SRCS

	"src/connection.hpp"
	"src/structs.hpp"
	"src/parser.hpp"
	"src/common.cpp"
	"src/connection.cpp"
	"src/parser.cpp"
)


add_library(nats_asio ${nats_asio_SRCS} ${nats_asio_PUB_INCLUDES})
target_link_libraries(nats_asio  pthread "${Boost_COROUTINE_LIBRARIES}" OpenSSL::Crypto OpenSSL::SSL fmt::fmt)

if (ENABLE_EXAMPLES)
	add_executable(example1 main.cpp)
	target_link_libraries(example1 nats_asio)
endif()

install(TARGETS nats_asio DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/")
install(FILES ${nats_asio_PUB_INCLUDES} DESTINATION "${CMAKE_INSTALL_PREFIX}/include/nats_asio/")

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


