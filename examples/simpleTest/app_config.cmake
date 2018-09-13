# allolib location
set(allolib_directory ../../../allolib)

set(BUILD_EXAMPLES 0)
add_subdirectory(../../../libdatk ../../../libdatk/build) 

# list your app files here
set(app_files_list
	main.cpp
)

get_target_property(DATK_LIBRARIES datk DATK_LIBRARIES)
get_target_property(DATK_INCLUDE_DIRS datk DATK_INCLUDE_DIRS)

#message("---...--- ${DATK_LIBRARIES}")

# other directories to include
set(app_include_dirs
    ../../include
    ${DATK_INCLUDE_DIRS}
)

# other libraries to link
set(app_link_libs
    ${DATK_LIBRARIES}
    $<TARGET_LINKER_FILE:datk> 
    datk
)

# definitions
set(app_definitions
)

# compile flags
set(app_compile_flags
)

# linker flags, with `-` in the beginning
set(app_linker_flags
)