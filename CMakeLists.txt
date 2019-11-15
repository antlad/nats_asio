cmake_minimum_required(VERSION 3.12)

project(nats_asio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS)


option(ENABLE_TESTS "enable tests" OFF)
option(ENABLE_EXAMPLES "enable build examples" OFF)

add_definitions(-DSPDLOG_FMT_EXTERNAL)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(ENABLE_CONAN_QTCREATOR_SUPPORT)
  include("${CMAKE_CURRENT_LIST_DIR}/cmake/conan.cmake")
  conan_check(REQUIRED)

  if (NOT "$ENV{CONAN_PROFILE}" STREQUAL "")
    set(CONAN_PROFILE "$ENV{CONAN_PROFILE}")
    message("conan is using profile ${CONAN_PROFILE}")

    conan_cmake_run(
    CONANFILE "conanfile.txt"
    PROFILE ${CONAN_PROFILE}
    BUILD missing
    BASIC_SETUP
    )
  else()
    message("conan is using default profile")

    conan_cmake_run(
    CONANFILE "conanfile.txt"
    PROFILE default
    BUILD missing
    BASIC_SETUP
    )
  endif()
  
else()
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
endif()




find_package(Boost COMPONENTS coroutine REQUIRED)
find_package(FMT REQUIRED)
find_package(spdlog REQUIRED)
#find_package(OpenSSL REQUIRED)

include_directories(
	"${CMAKE_CURRENT_LIST_DIR}/include"
	"${CMAKE_CURRENT_LIST_DIR}/src"
	)

set(LINCLUDE "${CMAKE_CURRENT_LIST_DIR}/include/nats_asio" )

set(nats_asio_PUB_INCLUDES "${LINCLUDE}/interface.hpp")

set(nats_asio_SRCS
	"src/connection.hpp"
	"src/parser.hpp"
	"src/connection.cpp"
	"src/parser.cpp"
	"src/socket.hpp"
)

add_executable(nats_tool nats_tool.cpp ${nats_asio_SRCS})
target_link_libraries(nats_tool ${CONAN_LIBS})

if (ENABLE_TESTS)
	enable_testing()
	find_package(GTest REQUIRED)
	get_filename_component(GTEST_LIB_FOLDER ${GTEST_LIBRARY} DIRECTORY)
	include_directories(${GTEST_INCLUDE_DIRS})
	link_directories(${GTEST_LIB_FOLDER})
	add_executable(parser_test tests/check1.cpp)
	target_link_libraries(parser_test GTest::GTest GTest::Main gmock gmock_main nats_asio "${Boost_COROUTINE_LIBRARIES}" OpenSSL::Crypto OpenSSL::SSL fmt::fmt)
	add_test(DefaultTestSuite default_test_suite)
endif()


