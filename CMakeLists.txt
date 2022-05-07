# version 3.11 or later of CMake needed later for installing GoogleTest
# so let's require it now.
cmake_minimum_required( VERSION 3.11 )

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/lib")

project( main_project
         VERSION 0.1
         DESCRIPTION "Main Project" )

set( MAIN_ROOT_SOURCE_DIR ${PROJECT_SOURCE_DIR} )

add_subdirectory( ${PROJECT_SOURCE_DIR}/external/libraries )  # look in hello subdirectory for CMakeLists.txt to process

add_subdirectory( ${PROJECT_SOURCE_DIR}/external/package_manager/conan )  # look in hello subdirectory for CMakeLists.txt to process

add_subdirectory( ${PROJECT_SOURCE_DIR}/application )  # look in apps subdirectory for CMakeLists.txt to process
