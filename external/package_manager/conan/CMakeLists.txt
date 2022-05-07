# version 3.11 or later of CMake needed later for installing GoogleTest
# so let's require it now.
cmake_minimum_required( VERSION 3.11 )

project( PACKAGE_MANAGER_CONAN_LIBS
         VERSION 0.1
         DESCRIPTION "Package Manager Conan library interface integration" )

# ******* conan package manager ********

#You must had installed conan in your system
#to check write in the vscode console conan
#to install look at google "conan c++ install"

# Download automatically, you can also just copy the conan.cmake file
if ( NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake" )
   message( STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan" )
   file( DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake" )
endif()

#Sometime you need delete all build folder and generate all build from scratch
# The cmake cache can be tricky!!!!
set( CONAN_DISABLE_CHECK_COMPILER True CACHE BOOL "" FORCE )

include( ${CMAKE_BINARY_DIR}/conan.cmake )

#add_definitions( -DRESTINIO_EXTERNAL_STRING_VIEW_LITE=1 )

# conan_cmake_run( REQUIRES soci/4.0.3
#                  BASIC_SETUP
#                  BUILD missing
#                  OPTIONS soci:shared=False
#                          soci:with_mysql=True )
                # BUILD soci
                #BUILD libmysqlclient )

# conan_cmake_run( REQUIRES restinio/0.6.14
#                  BASIC_SETUP
#                  BUILD missing )

conan_cmake_run( REQUIRES mongo-cxx-driver/3.6.6
                 BASIC_SETUP
                 CMAKE_BUILD_TYPE=DEBUG
                 BUILD missing )
#                 BUILD all ) #Force to build local from the source code. Avoid valgrind errors about cannot locate the debug info

# conan_cmake_run( REQUIRES nlohmann_json/3.10.4
#                  BASIC_SETUP )

conan_cmake_run( REQUIRES fmt/8.0.1
                 BASIC_SETUP
                 CMAKE_BUILD_TYPE=DEBUG
                 BUILD missing )
#                 BUILD all ) #Force to build local from the source code. Avoid valgrind errors about cannot locate the debug info

conan_cmake_run( REQUIRES xxhash/0.8.1
                 BASIC_SETUP
                 CMAKE_BUILD_TYPE=DEBUG
                 BUILD missing )
#                 BUILD all ) #Force to build local from the source code. Avoid valgrind errors about cannot locate the debug info

conan_cmake_run( REQUIRES redis-plus-plus/1.3.3
                 BASIC_SETUP
                 CMAKE_BUILD_TYPE=DEBUG
                 BUILD missing )
#                 BUILD all ) #Force to build local from the source code. Avoid valgrind errors about cannot locate the debug info

# conan_cmake_run( REQUIRES rapidjson/cci.20211112
#                  BASIC_SETUP )

# conan_cmake_run( REQUIRES spdlog/1.9.2
#                  BASIC_SETUP
#                  BUILD missing )

add_library( ${PROJECT_NAME} INTERFACE )

message( STATUS "****** => PACKAGE_MANAGER_CONAN_LIBS" )
message( STATUS "****** => From cmake file: [project_root]/external/libraries/package_manager/conan/CMakeList.txt" )
message( STATUS "****** => CONAN_INCLUDE_DIRS=${CONAN_INCLUDE_DIRS}" )
message( STATUS "****** => CONAN_LIB_DIRS=${CONAN_LIB_DIRS}" )
message( STATUS "****** => CONAN_LIBS=${CONAN_LIBS}" )

target_include_directories( ${PROJECT_NAME} INTERFACE ${CONAN_INCLUDE_DIRS} )

#VERY IMPORTANT!!!. Pass to linker the folders for lookup the libraries in link stage.
target_link_directories( ${PROJECT_NAME} INTERFACE ${CONAN_LIB_DIRS} )
target_link_libraries( ${PROJECT_NAME} INTERFACE ${CONAN_LIBS} ssl crypto dl m z )

# target_compile_definitions( ${PROJECT_NAME} INTERFACE RESTINIO_EXTERNAL_STRING_VIEW_LITE=1
#                                                       RESTINIO_EXTERNAL_OPTIONAL_LITE=1
#                                                       RESTINIO_EXTERNAL_VARIANT_LITE=1
#                                                       RESTINIO_EXTERNAL_EXPECTED_LITE=1 )

# ******* conan package manager ********
