message(STATUS "Building DBOS scheduler libs/lib_util")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Dependencies
# Find boost static libs, statically linked to C++ runtime.
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(Boost 1.53 COMPONENTS system thread)
message(STATUS "Using Boost ${Boost_VERSION}")

set(lib_util_SOURCES BenchmarkUtil.cc)
add_library(lib_util STATIC ${lib_util_SOURCES})

target_include_directories(lib_util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(lib_util
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
