# version 3.11 or later of CMake or needed later for installing GoogleTest
# so let's require it now.
cmake_minimum_required( VERSION 3.11-3.18 )

project( main )

set( CMAKE_FIND_LIBRARY_SUFFIXES ".a" ) # <--- Change the preference to .a and not .so/.dll
#set( BUILD_SHARED_LIBS OFF )
#set( CMAKE_EXE_LINKER_FLAGS "-static -Wl,--start-group" ) # <-- full static executable
set( CMAKE_EXE_LINKER_FLAGS "-Wl,--start-group" ) # <-- full static executable
#set( CMAKE_EXE_LINKER_FLAGS "-fsanitize=address" ) # <-- active the address sanitizer in development
#set( CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++" ) # valgrind detect a memoty leak of 72 kb. Is ok is gnu standar library c++ memory leak

SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native" ) #Required by uuid gen

#file( GLOB_RECURSE SOURCES "src/*.cpp" )
file( GLOB_RECURSE SOURCES "src/main_01.cpp" )
#file( GLOB_RECURSE SOURCES "src/main_02.cpp" )
file( GLOB_RECURSE MODULES_SOURCES "src/modules/*.cpp" )
file( GLOB_RECURSE MODULES_INCLUDES_H "src/modules/*.h" )
file( GLOB_RECURSE MODULES_INCLUDES_HPP "src/modules/*.hpp" )

file( GLOB_RECURSE CONFIGS "config/*.*" )

file( COPY ${CONFIGS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config )

add_executable( ${PROJECT_NAME} ${MODULES_SOURCES} ${MODULES_INCLUDES_H} ${MODULES_INCLUDES_HPP} ${SOURCES} )

target_include_directories( ${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src/include )

#-static <-- active the full static linker
#target_link_libraries( ${PROJECT_NAME} PRIVATE BCRYPT NANOJSON UUID_V4 HV -static )
#target_link_libraries( ${PROJECT_NAME} PRIVATE BCRYPT NANOJSON UUID_V4 HV -static-libasan )
target_link_libraries( ${PROJECT_NAME} PRIVATE BCRYPT NANOJSON UUID_V4 HV MYSQL_CLIENT MYSQL_SERVICES SOCI_CORE SOCI_MYSQL )

# ***************** Conan Integration ***************

target_link_libraries( ${PROJECT_NAME} PRIVATE PACKAGE_MANAGER_CONAN_LIBS )

# ***************** Conan Integration ***************

# -static-libgcc -static-libstdc++

# Tell C++ compiler to use C++20 features. We don't actually use any of them.
target_compile_features( ${PROJECT_NAME} PUBLIC cxx_std_20 )
#target_compile_options(  ${PROJECT_NAME} PRIVATE -fsanitize=address )
#target_link_options( ${PROJECT_NAME} PRIVATE -fsanitize=address )
