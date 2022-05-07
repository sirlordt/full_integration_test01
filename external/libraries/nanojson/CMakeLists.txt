# version 3.11 or later of CMake or needed later for installing GoogleTest
# so let's require it now.
cmake_minimum_required( VERSION 3.11 )

project( NANOJSON )

add_library( ${PROJECT_NAME} INTERFACE )

#set_target_properties(${PROJECT_NAME} PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/static/libmysqlclient.a)

#set_target_properties(${PROJECT_NAME} PROPERTIES INTERFACE_LINK_LIBRARIES ssl)
#set_target_properties(${PROJECT_NAME} PROPERTIES INTERFACE_LINK_LIBRARIES pthread)

target_include_directories( ${PROJECT_NAME} INTERFACE ${PROJECT_SOURCE_DIR}/include )

#Work !!!
#include_directories( ${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

#target_link_libraries(${PROJECT_NAME} INTERFACE ssl crypto dl m z )

#project(mysql_services)

#add_library(${PROJECT_NAME} STATIC IMPORTED GLOBAL)
#set_target_properties(${PROJECT_NAME} PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/static/libmysqlservices.a)

