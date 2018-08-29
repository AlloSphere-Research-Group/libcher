cmake_minimum_required(VERSION 3.9)

project(libdatk VERSION 1.0.0 DESCRIPTION "Distributed multimedia framework")

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_BINARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build)

# ZeroMQ

if(WIN32)
    set (ZeroMQ_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/zeromq/include)
    find_library (ZeroMQ_LIBRARY NAMES libzmq PATHS ${CMAKE_CURRENT_SOURCE_DIR}/external/zeromq/lib SHARED NO_DEFAULT_PATH NO_SYSTEM_ENVIRONMENT_PATH)

#    message("==== ${CMAKE_SOURCE_DIR}/external/zeromq ${ZeroMQ_LIBRARY}")
else()
    set (ZeroMQ_ROOT_DIR "${CMAKE_SOURCE_DIR}/external/zeromq")
    find_package(ZeroMQ QUIET)
    if (NOT ZeroMQ_FOUND)
        add_subdirectory(external/libzmq)
#       add_subdirectory(external/cppzmq)
    endif()
endif()

# libdatk library
add_library(datk STATIC
    include/datk.hpp
    include/nodeprocess.hpp

    src/nodeprocess.cpp
)

target_link_libraries(datk ${ZeroMQ_LIBRARY})

# Set target properties
set_target_properties(datk PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(datk PROPERTIES SOVERSION 1)
set_target_properties(datk PROPERTIES PUBLIC_HEADER include/datk.hpp)

target_include_directories(datk PRIVATE .)
target_include_directories(datk PRIVATE include)
target_include_directories(datk PRIVATE src)
target_include_directories(datk PRIVATE ${ZeroMQ_INCLUDE_DIR})

set_target_properties(datk PROPERTIES DATK_LIBRARIES ${ZeroMQ_LIBRARY})
set_target_properties(datk PROPERTIES DATK_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include ${ZeroMQ_INCLUDE_DIR}")
set_target_properties(datk PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_OUTPUT_DIRECTORY})
set_target_properties(datk PROPERTIES ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_OUTPUT_DIRECTORY})

#install(TARGETS datk
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# examples
add_executable(simpleTest examples/simpleTest/main.cpp)
target_link_libraries (simpleTest LINK_PUBLIC $<TARGET_LINKER_FILE:datk> datk)
add_dependencies(simpleTest datk)
link_directories(${CMAKE_BINARY_OUTPUT_DIRECTORY})
target_include_directories(simpleTest PRIVATE ${ZeroMQ_INCLUDE_DIR} include)
