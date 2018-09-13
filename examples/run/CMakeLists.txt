cmake_minimum_required(VERSION 3.0)
set(app_path ${CMAKE_SOURCE_DIR})
set(app_name datkrun)
project(${app_name})

set(app_files_list
	main.cpp
)

add_executable(${app_name} ${app_files_list})

set_target_properties(${app_name} PROPERTIES
  DEBUG_POSTFIX _debug
  RUNTIME_OUTPUT_DIRECTORY ${app_path}/bin
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${app_path}/bin
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${app_path}/bin
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
)

# flags
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_compile_options(${app_name} PRIVATE "")
else ()
  target_compile_options(${app_name} PRIVATE "-Wall")
endif ()

get_target_property(DATK_LIBRARIES datk DATK_LIBRARIES)
get_target_property(DATK_INCLUDE_DIRS datk DATK_INCLUDE_DIRS)

message (${DATK_LIBRARIES})
link_directories(../../build)

target_include_directories(${app_name} PRIVATE ${CMAKE_SOURCE_DIR}/include ${DATK_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/external)
target_link_libraries(${app_name} ${app_link_libs} datk 
    ${DATK_LIBRARIES}
    $<TARGET_LINKER_FILE:datk>)
target_compile_definitions(${app_name} PRIVATE ${app_definitions})
target_compile_options(${app_name} PRIVATE ${app_compile_flags})
# Item names starting with -, but not -l or -framework, are treated as linker flags.
target_link_libraries(${app_name} ${app_linker_flags})

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # when run from Visual Studio, working directory is where the solution is by default
  # set it to app output directory
  set_target_properties(${app_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${app_path}/bin)
  # startup project is `ALL_BUILD` by default so we change it to app project
  set_directory_properties(PROPERTIES VS_STARTUP_PROJECT bin)
endif ()