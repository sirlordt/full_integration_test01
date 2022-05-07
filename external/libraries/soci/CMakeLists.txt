cmake_minimum_required(VERSION 3.11-3.18)

project( SOCI )

# MAIN_ROOT_SOURCE_DIR come from the top most CMakeLists.txt file
message( STATUS "MAIN_ROOT_SOURCE_DIR=${MAIN_ROOT_SOURCE_DIR}" )
#message(STATUS "CMAKE_SYSTEM_INCLUDE_PATH=${CMAKE_SYSTEM_INCLUDE_PATH}")

# Emulate the ExternalProject_Add but in config time.
# Because the Fetch_Declare, Fetch_Populate, Fetch_* no work. Never create the static libraries.
if ( NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/build" OR
     NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/install" OR
     FORCE_BUILD_INSTALL_SOCI_LIBRARY )

  if ( NOT EXISTS "${PROJECT_SOURCE_DIR}/git_repo/src/CMakeLists.txt" )

    message(STATUS "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")
    execute_process( COMMAND git clone https://github.com/sirlordt/soci ${PROJECT_SOURCE_DIR}/git_repo )
    execute_process( COMMAND git -C ${PROJECT_SOURCE_DIR}/git_repo checkout soci_my_merged_features )

  endif()

  file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/install )
  file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build )

  set(ENV{NO_COLOR} 1)
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
add_library( SOCI_CORE STATIC IMPORTED GLOBAL )
set_target_properties( SOCI_CORE PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libsoci_core.a )

target_include_directories( SOCI_CORE INTERFACE
                            ${CMAKE_CURRENT_BINARY_DIR}/install/include )

add_library( SOCI_MYSQL STATIC IMPORTED GLOBAL )
set_target_properties( SOCI_MYSQL PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libsoci_mysql.a )

# Ubuntu package mysqlclient-dev only had support to pkgconfig not for cmake.
# We cannot use findPackage(MySQL)
include(FindPkgConfig)

#pkg_check_modules(LIBMYSQLCLIENT REQUIRED mysqlclient)

pkg_get_variable( MYSQL_INCLUDE_PATH mysqlclient includedir )

message( STATUS "MYSQL_INCLUDE_PATH=${MYSQL_INCLUDE_PATH}" )

# The next are required by soci. Because in the mysql backend the include
# are "#include <mysql.h>" and not "#include <mysql/mysql.h>"
target_include_directories( SOCI_MYSQL INTERFACE ${MYSQL_INCLUDE_PATH} )

#target_include_directories( SOCI_MYSQL INTERFACE /usr/include/mysql )

target_link_libraries( SOCI_MYSQL INTERFACE SOCI_CORE MYSQL_CLIENT MYSQL_SERVICES )
