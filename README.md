# cppdbc_integration_test01

Example Project To Play With CMake, Conan, CPP, HV, MongoDB, BCRYPT, xxHash

Look at external/package_manager/conan/CMakeLists.txt for advance CMake/Conan Integration

* mongodb
* libHV
* bCrypt
* xxHash
* CMake
* Conan
* conan_cmake_run
* CONAN_DISABLE_CHECK_COMPILER
* g++11

CMakeList.txt external/package_manager/conan/CMakeLists.txt

```cmake

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

if ( NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake" )
   message( STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan" )
   file( DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                  "${CMAKE_BINARY_DIR}/conan.cmake" )
endif()

#Sometime you need delete all build folder and generate all build from scratch
set( CONAN_DISABLE_CHECK_COMPILER True CACHE BOOL "" FORCE )

include( ${CMAKE_BINARY_DIR}/conan.cmake )

#add_definitions( -DRESTINIO_EXTERNAL_STRING_VIEW_LITE=1 )

#conan_cmake_run( REQUIRES restinio/0.6.14
#                 BASIC_SETUP
#                 BUILD missing )

# conan_cmake_run( REQUIRES mongo-cxx-driver/3.6.6
#                  BASIC_SETUP
#                  BUILD missing )

# conan_cmake_run( REQUIRES nlohmann_json/3.10.4
#                  BASIC_SETUP )

conan_cmake_run( REQUIRES fmt/8.0.1
                 BASIC_SETUP
#                 SETTINGS compiler.version=9.3;
                 BUILD missing )

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
target_link_libraries( ${PROJECT_NAME} INTERFACE ${CONAN_LIBS} )

target_compile_definitions( ${PROJECT_NAME} INTERFACE RESTINIO_EXTERNAL_STRING_VIEW_LITE=1 RESTINIO_EXTERNAL_OPTIONAL_LITE=1 RESTINIO_EXTERNAL_VARIANT_LITE=1 RESTINIO_EXTERNAL_EXPECTED_LITE=1 )

# ******* conan package manager ********

```

CMakeList.txt external/libraries/soci/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.11-3.18)

project( soci )

# MAIN_ROOT_SOURCE_DIR come from the top most CMakeLists.txt file
message(STATUS "MAIN_ROOT_SOURCE_DIR=${MAIN_ROOT_SOURCE_DIR}")
#message(STATUS "CMAKE_SYSTEM_INCLUDE_PATH=${CMAKE_SYSTEM_INCLUDE_PATH}")

# Emulate the ExternalProject_Add but in config time.
# Because the Fetch_Declare, Fetch_Populate, Fetch_* no work. Never create the static libraries.
if ( NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/build" OR
     NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/install" OR
     FORCE_BUILD_INSTALL_SOCI_LIBRARY )

  if ( NOT EXISTS "${PROJECT_SOURCE_DIR}/git_repo/src/CMakeLists.txt" )

    execute_process( COMMAND git clone https://github.com/SOCI/soci ${PROJECT_SOURCE_DIR}/git_repo )

  endif()

  file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/install )
  file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build )

  execute_process( COMMAND cmake ${CMAKE_CURRENT_SOURCE_DIR}/git_repo -DSOCI_CXX11:BOOL=ON
                                                                      -DSOCI_TESTS:BOOL=OFF
                                                                      -DWITH_MYSQL:BOOL=ON
                                                                      # The CMAKE_COLOR_MAKEFILE in off.
                                                                      # Not work for the soci config messages
                                                                      -DCMAKE_COLOR_MAKEFILE:BOOL=OFF
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build )

  execute_process( COMMAND cmake --build ${CMAKE_CURRENT_BINARY_DIR}/build )

  execute_process( COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}/build
                                 --prefix "${CMAKE_CURRENT_BINARY_DIR}/install" )

endif()

# Manual create the target soci_core and soci_mysql
add_library( soci_core STATIC IMPORTED GLOBAL )
set_target_properties( soci_core PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libsoci_core.a )

target_include_directories( soci_core INTERFACE
                            ${CMAKE_CURRENT_BINARY_DIR}/install/include )

add_library( soci_mysql STATIC IMPORTED GLOBAL )
set_target_properties( soci_mysql PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libsoci_mysql.a )

# Ubuntu package mysqlclient-dev only had support to pkgconfig not for cmake.
# We cannot use findPackage(MySQL)
include(FindPkgConfig)

#pkg_check_modules(LIBMYSQLCLIENT REQUIRED mysqlclient)

pkg_get_variable(MYSQL_INCLUDE_PATH mysqlclient includedir)

message(STATUS "MYSQL_INCLUDE_PATH=${MYSQL_INCLUDE_PATH}")

# The next are required by soci. Because in the mysql backend the include
# are "#include <mysql.h>" and not "#include <mysql/mysql.h>"
target_include_directories( soci_mysql INTERFACE ${MYSQL_INCLUDE_PATH} )

#target_include_directories( soci_mysql INTERFACE /usr/include/mysql )

target_link_libraries( soci_mysql INTERFACE soci_core mysql_client mysql_services )
```
