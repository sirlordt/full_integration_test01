cmake_minimum_required(VERSION 3.11-3.18)

project( BCRYPT )

# MAIN_ROOT_SOURCE_DIR come from the top most CMakeLists.txt file
message(STATUS "MAIN_ROOT_SOURCE_DIR=${MAIN_ROOT_SOURCE_DIR}")
#message(STATUS "CMAKE_SYSTEM_INCLUDE_PATH=${CMAKE_SYSTEM_INCLUDE_PATH}")

# Emulate the ExternalProject_Add but in config time.
# Because the Fetch_Declare, Fetch_Populate, Fetch_* no work. Never create the static libraries.
if ( NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/build" OR
     NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/install" OR
     FORCE_BUILD_INSTALL_SOCI_LIBRARY )

  if ( NOT EXISTS "${PROJECT_SOURCE_DIR}/git_repo/src/CMakeLists.txt" )

    message(STATUS "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}")
    execute_process( COMMAND git clone https://github.com/hilch/Bcrypt.cpp ${PROJECT_SOURCE_DIR}/git_repo )
    #execute_process( COMMAND git -C ${PROJECT_SOURCE_DIR}/git_repo checkout soci_my_merged_features )

  endif()

  file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/install )
  file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build )

  set(ENV{NO_COLOR} 1)
  execute_process( COMMAND cmake ${CMAKE_CURRENT_SOURCE_DIR}/git_repo -DCMAKE_COLOR_MAKEFILE:BOOL=OFF
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build )

  execute_process( COMMAND cmake --build ${CMAKE_CURRENT_BINARY_DIR}/build )

  file( COPY ${CMAKE_CURRENT_BINARY_DIR}/build/libbcrypt.a
             DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install/lib/  )

  file( GLOB_RECURSE BCRYPT_HEADERS
                     ${PROJECT_SOURCE_DIR}/git_repo/include/*.h )

  file( COPY ${BCRYPT_HEADERS}
             DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/install/include/bcrypt/ )

  #cmake --install . --prefix ./install
  # execute_process( COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}/build
  #                                --prefix "${CMAKE_CURRENT_BINARY_DIR}/install" )

endif()

# Manual create the target to libhv
add_library( BCRYPT STATIC IMPORTED GLOBAL )
set_target_properties( BCRYPT PROPERTIES IMPORTED_LOCATION
                       ${CMAKE_CURRENT_BINARY_DIR}/install/lib/libbcrypt.a )

target_include_directories( BCRYPT INTERFACE
                            ${CMAKE_CURRENT_BINARY_DIR}/install/include )

target_link_libraries( BCRYPT INTERFACE pthread )
