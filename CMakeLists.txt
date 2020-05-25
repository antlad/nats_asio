cmake_minimum_required(VERSION 3.12)

project(nats_asio)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TESTS "enable tests" OFF)
option(BUILD_NATS_TOOL "build nats tool" OFF)
option(USE_CONAN "use conan for deps" ON)

add_definitions(-DSPDLOG_FMT_EXTERNAL)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if (USE_CONAN)
    if (ENABLE_CONAN_QTCREATOR_SUPPORT)
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
        else ()
            message("conan is using default profile")

            conan_cmake_run(
                    CONANFILE "conanfile.txt"
                    PROFILE default
                    BUILD missing
                    BASIC_SETUP
            )
        endif ()

    else ()
        include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
        conan_basic_setup()
    endif ()
endif()

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
    target_link_libraries(parser_test ${CONAN_LIBS})
    add_test(DefaultTestSuite default_test_suite)
endif ()


