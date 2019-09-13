cmake_minimum_required(VERSION 3.12)

project(nats-asio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_TESTS "enable tests" OFF)

#add_definitions(-DSPDLOG_FMT_EXTERNAL)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
add_definitions(-DBOOST_ALL_DYN_LINK)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Boost 1.71 COMPONENTS coroutine REQUIRED)
find_package(FMT REQUIRED)
find_package(spdlog REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
	    "${CMAKE_INSTALL_PREFIX}/include"
		"${CMAKE_PREFIX_PATH}/include"
		"${Boost_INCLUDE_DIRS}"
		"${OPENSSL_INCLUDE_DIR}"
		)


#find_package(FMT REQUIRED)
#find_package(spdlog REQUIRED)
#find_package(FlatBuffers REQUIRED)
#find_package(gRPC REQUIRED)

include_directories(
	"${CMAKE_INSTALL_PREFIX}/include"
	"${CMAKE_PREFIX_PATH}/include"
	)

set(nats_asio_SRCS
	defs.hpp
	nats_client.hpp
	nats_client.cpp
	client.cpp
	client.hpp
)


add_library(nats_asio ${nats_asio_SRCS})
add_executable(nats_asio_check main.cpp)

target_link_libraries(nats_asio_check  pthread nats_asio "${Boost_COROUTINE_LIBRARIES}" OpenSSL::Crypto OpenSSL::SSL)

#if (ENABLE_TESTS)
#        enable_testing()
#        find_package(GTest REQUIRED)
#        include_directories(${GTEST_INCLUDE_DIRS})
#endif()

#if (ENABLE_TESTS)
    #enable_testing()
#	find_package(GTest REQUIRED)
    #include_directories(${GTEST_INCLUDE_DIRS})
#	add_executable(default_test_suite unit_tests/check1.cpp)
    #target_link_libraries(default_test_suite GTest::GTest GTest::Main)
#	add_test(DefaultTestSuite default_test_suite)
#endif()


