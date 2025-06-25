add_library(usermod_qspi INTERFACE)

# Add our source files to the lib
target_sources(usermod_qspi INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/qspi.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_qspi INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}

)
target_compile_definitions(usermod_qspi INTERFACE
    MODULE_QSPI_ENABLED=1
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_qspi)
