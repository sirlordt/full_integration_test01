# version 3.11 or later of CMake or needed later for installing GoogleTest
# so let's require it now.
cmake_minimum_required( VERSION 3.11 )

#Taked from repo: https://github.com/crashoz/uuid_v4

project( UUID_V4 )

add_library( ${PROJECT_NAME} INTERFACE )

target_include_directories( ${PROJECT_NAME} INTERFACE ${PROJECT_SOURCE_DIR}/include )

