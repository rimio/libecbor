cmake_minimum_required (VERSION 3.1)

project (ecbor LANGUAGES C CXX)

# Version
set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)
set (VERSION_PATCH 0)

# Options
option (BUILD_DESCRIBE_TOOL "build ecbor-describe" ON)
option (TESTING "build unit test targets" OFF)

# Testing dependencies
if (TESTING)
    find_package(GTest)
endif()

# Compiler setup
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options (
    -Wall
    -Wextra
    -pedantic
    $<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>
    $<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>
    $<$<COMPILE_LANGUAGE:C>:-Wold-style-definition>)

# Output directory setup
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

# Directory setup
set (SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set (INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

include_directories (${INCLUDE_DIR} ${SOURCE_DIR})

# Configurations
configure_file (
  "${INCLUDE_DIR}/ecbor.h.in"
  "${INCLUDE_DIR}/ecbor.h"
)

# Source setup
set (LIB_INCLUDES
  "${INCLUDE_DIR}/ecbor.h"
)

set (LIB_SOURCES
  "${SRC_DIR}/libecbor/ecbor.c"
  "${SRC_DIR}/libecbor/ecbor_encoder.c"
  "${SRC_DIR}/libecbor/ecbor_decoder.c"
)

set (DESCRIBE_TOOL_SOURCES
  "${SRC_DIR}/ecbor-describe/ecbor_describe.c"
)

# Targets
add_library (${PROJECT_NAME}_shared SHARED ${LIB_SOURCES})
add_library (${PROJECT_NAME}_static STATIC ${LIB_SOURCES})

set_target_properties (${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME} PUBLIC_HEADER ${LIB_INCLUDES} LINKER_LANGUAGE C)
set_target_properties (${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME} PUBLIC_HEADER ${LIB_INCLUDES} LINKER_LANGUAGE C)

target_compile_options (${PROJECT_NAME}_static PRIVATE -nostdlib)

install (TARGETS ${PROJECT_NAME}_shared LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include)
install (TARGETS ${PROJECT_NAME}_static ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)

# Tool Targets
if (BUILD_DESCRIBE_TOOL)
  add_executable (${PROJECT_NAME}-describe ${DESCRIBE_TOOL_SOURCES})
  target_link_libraries (${PROJECT_NAME}-describe ${PROJECT_NAME}_shared)
  install (TARGETS ${PROJECT_NAME}-describe DESTINATION bin)
endif (BUILD_DESCRIBE_TOOL)

# Test targets
if (TESTING)
    set (UNIT_TEST_SOURCES
        "${SRC_DIR}/unittest/test.cpp"
        "${SRC_DIR}/unittest/test_encoder.cpp"
    )

    # Unit tests
    enable_testing ()
    add_executable(unittest ${UNIT_TEST_SOURCES})
    target_link_libraries (unittest GTest::GTest ${PROJECT_NAME}_static)
    gtest_discover_tests (unittest)
endif()
