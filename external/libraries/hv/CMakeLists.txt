cmake_minimum_required(VERSION 3.11-3.18)

project( HV )

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a") # <--- Change the preference to .a and not .so/.dll

# MAIN_ROOT_SOURCE_DIR come from the top most CMakeLists.txt file
message(STATUS "MAIN_ROOT_SOURCE_DIR=${MAIN_ROOT_SOURCE_DIR}")
#message(STATUS "CMAKE_SYSTEM_INCLUDE_PATH=${CMAKE_SYSTEM_INCLUDE_PATH}")

# Emulate the ExternalProject_Add but in config time.
# Because the Fetch_Declare, Fetch_Populate, Fetch_* no work. Never create the static libraries.
if ( NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/build" OR
     NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/install" OR
     FORCE_BUILD_INSTALL_SOCI_LIBRARY )

  if ( NOT EXISTS "${PROJECT_SOURCE_DIR}/git_repo/CMakeLists.txt" )

    message(STATUS "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")
    #execute_process( COMMAND git clone https://github.com/ithewei/libhv ${PROJECT_SOURCE_DIR}/git_repo )
    execute_process( COMMAND git clone https://github.com/sirlordt/libhv ${PROJECT_SOURCE_DIR}/git_repo )
    execute_process( COMMAND git -C ${PROJECT_SOURCE_DIR}/git_repo checkout main_ctx_finish )

  endif()

  file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/install )
  file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build )

  set(ENV{NO_COLOR} 1)
  execute_process( COMMAND cmake ${CMAKE_CURRENT_SOURCE_DIR}/git_repo -DBUILD_EXAMPLES:BOOL=OFF
                                                                      -DBUILD_UNITTEST:BOOL=OFF
                                                                      -DWITH_PROTOCOL:BOOL=ON
                                                                      -DCMAKE_COLOR_MAKEFILE:BOOL=OFF
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build )

  execute_process( COMMAND cmake --build ${CMAKE_CURRENT_BINARY_DIR}/build )

  execute_process( COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}/build
                                 --prefix "${CMAKE_CURRENT_BINARY_DIR}/install" )

endif()

# Manual create the target to libhv
add_library( HV STATIC IMPORTED GLOBAL )
set_target_properties( HV PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libhv_static.a )

target_include_directories( HV INTERFACE
                            ${CMAKE_CURRENT_BINARY_DIR}/install/include )

target_link_libraries( HV INTERFACE pthread )
