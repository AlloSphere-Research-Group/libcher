cmake_minimum_required(VERSION 3.9)

project(libdatk VERSION 1.0.0 DESCRIPTION "Distributed multimedia framework")

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_BINARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# ZeroMQ

if(WIN32)
    set (ZeroMQ_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/zeromq/include)
    find_library (ZeroMQ_LIBRARY NAMES libzmq PATHS ${CMAKE_CURRENT_SOURCE_DIR}/external/zeromq/lib SHARED NO_DEFAULT_PATH NO_SYSTEM_ENVIRONMENT_PATH)
    
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/zeromq/include )
    message("==== ${CMAKE_SOURCE_DIR}/external/zeromq ${ZeroMQ_LIBRARY}")
else()
    set (ZeroMQ_ROOT_DIR "${CMAKE_SOURCE_DIR}/external/zeromq")
    find_package(ZeroMQ QUIET)
    if (NOT ZeroMQ_FOUND)
        add_subdirectory(external/libzmq)
        
#       add_subdirectory(external/cppzmq)
    endif()
endif()

# Libssh

# TODO handle zlib better
set(WITH_ZLIB OFF)
set(WITH_EXAMPLES OFF) # Disables example building for libssh
add_subdirectory(external/libssh)
include_directories(external/libssh/include)

# json
include_directories(external)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_definitions(-DDATK_WINDOWS)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    add_definitions(-DDATK_OSX)
    add_definitions(-DDATK_UNIX)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_definitions(-DDATK_LINUX)
    add_definitions(-DDATK_UNIX)
endif()

# libdatk library
add_library(datk STATIC
    include/datk.hpp
    include/nodeio.hpp
    include/nodecontroller.hpp
    include/nodeprocess.hpp
    
    src/nodeio.cpp
    src/nodeprocess.cpp
    src/nodecontroller.cpp
)

target_link_libraries(datk ${ZeroMQ_LIBRARY})

# Set target properties
set_target_properties(datk PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/datk.hpp
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_OUTPUT_DIRECTORY}
    ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_OUTPUT_DIRECTORY}
    DEBUG_POSTFIX _debug
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(datk PROPERTIES 
    DATK_LIBRARIES ${ZeroMQ_LIBRARY}
    DATK_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include ${ZeroMQ_INCLUDE_DIR}"
)

target_include_directories(datk PRIVATE . include src ${ZeroMQ_INCLUDE_DIR})

#install(TARGETS datk
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# examples
add_executable(simpleTest examples/simpleTest/main.cpp)
target_link_libraries (simpleTest LINK_PUBLIC $<TARGET_LINKER_FILE:datk> datk)

set_target_properties(simpleTest PROPERTIES
  DEBUG_POSTFIX _debug
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_OUTPUT_DIRECTORY}
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_OUTPUT_DIRECTORY}
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_OUTPUT_DIRECTORY}
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
)
add_dependencies(simpleTest datk)
link_directories(${CMAKE_BINARY_OUTPUT_DIRECTORY})
target_include_directories(simpleTest PRIVATE ${ZeroMQ_INCLUDE_DIR} include)

add_subdirectory(examples/run)