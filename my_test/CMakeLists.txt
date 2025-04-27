cmake_minimum_required(VERSION 3.10)

project(flashdb_test LANGUAGES C CXX)

# Set the C standard (optional, but recommended if you want at least C99)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FLASHDB_LIB_PATH "${CMAKE_CURRENT_LIST_DIR}/../")
add_subdirectory(
  "${FLASHDB_LIB_PATH}"
  "${CMAKE_CURRENT_BINARY_DIR}/flashdb"
)

#######=================Test executable=================###########################################################
set(TEST_EXECUTABLE_NAME ${PROJECT_NAME}_exe)
# Source file(s)
set(SOURCES
    main.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create an executable from the specified source files
add_executable(${TEST_EXECUTABLE_NAME} ${SOURCES})
target_compile_options(${TEST_EXECUTABLE_NAME} PRIVATE -O0 -g)

# If you need any libraries, you can link them here, e.g.,
# target_link_libraries(${TEST_EXECUTABLE_NAME} rt)    # example for -lrt
target_link_libraries(${TEST_EXECUTABLE_NAME} pthread) # example for -lpthread
target_link_libraries(${TEST_EXECUTABLE_NAME} flashdb)

# Optionally set any additional compile flags
#set_target_properties(${TEST_EXECUTABLE_NAME} PROPERTIES COMPILE_FLAGS "-Wall -Wextra")


######=================Test Pybind11 Library=================######################################################
# Below pybind11 module is needed by pybind11_add_module
set(PYBIND11_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../pybind11")
add_subdirectory(
  "${PYBIND11_MODULE_PATH}"
  "${CMAKE_CURRENT_BINARY_DIR}/pybind11"
)
# pybind11_add_module is built on top of CMake's add_library function. It creates a shared library
# with the given name from the provided source files (PYBIND11_MODULE). The shared library is then automatically
# loaded into the Python interpreter when the module is imported.
# NOTE: this module name must be identical with that one specified in PYBIND11_MODULE(flashdb_test_lib, m)
set(PYBIND11_MODULE_NAME ${PROJECT_NAME}_lib)
pybind11_add_module(${PYBIND11_MODULE_NAME} pybind11_lib.cpp)

# When use target_link_libraries, put all the to link libraries in the same target_link_libraries command. So all have the same visibility.
# If you put them in different target_link_libraries commands, all should use the same visibility.
target_link_libraries(${PYBIND11_MODULE_NAME} PUBLIC flashdb pthread)
# flasddb relies on fal_cfg.h and fdb_cfg.h, need to make sure they are included
target_include_directories(flashdb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(TARGET ${PYBIND11_MODULE_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PYBIND11_MODULE_NAME}> ${CMAKE_CURRENT_SOURCE_DIR}/
  COMMENT "Copying ${PYBIND11_MODULE_NAME} to python directory"
)

message(STATUS "CMake project configured successfully.")
