cmake_minimum_required(VERSION 3.10)

project(flashdb_test LANGUAGES C CXX)

# Set the C standard (optional, but recommended if you want at least C99)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source file(s)
set(SOURCES
    main.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create an executable from the specified source files
add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g)

# If you need any libraries, you can link them here, e.g.,
# target_link_libraries(${PROJECT_NAME} rt)    # example for -lrt
target_link_libraries(${PROJECT_NAME} pthread) # example for -lpthread

# Optionally set any additional compile flags
#set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "-Wall -Wextra")

set(FLASHDB_LIB_PATH "${CMAKE_CURRENT_LIST_DIR}/../")
add_subdirectory(
  "${FLASHDB_LIB_PATH}"
  "${CMAKE_CURRENT_BINARY_DIR}/flashdb"
)
target_link_libraries(${PROJECT_NAME} flashdb)

message(STATUS "CMake project configured successfully.")
