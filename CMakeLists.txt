cmake_minimum_required(VERSION 3.10)
project(flashdb LANGUAGES C)
set(THIS_MODULE_NAME ${PROJECT_NAME})

file(
    GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/port/fal/src/*.c"
)

list(
    REMOVE_DUPLICATES SOURCES
)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    add_compile_definitions(
        FAL_DEBUG
    )
endif()

add_library(${THIS_MODULE_NAME} STATIC
    ${SOURCES}
)

# Specify include directories for the library
# This makes the header files available to targets linking against this library.
set(INCLUDE_DIRS 
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/port/fal/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/my_port/inc"
)
target_include_directories(${THIS_MODULE_NAME} PUBLIC
    ${INCLUDE_DIRS}
)

# There are some format warnings in the FlashDB library that we want to ignore
target_compile_options(${THIS_MODULE_NAME} PRIVATE -Wno-format)
