CMAKE_MINIMUM_REQUIRED(VERSION 2.8)


project(stem3)

#set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/install)
message("-- installation directory ${CMAKE_INSTALL_PREFIX}")


add_definitions(-std=c++11)
add_definitions(-DBOOST_LOG_DYN_LINK=1)
add_definitions(-DBOOST_TEST_DYN_LINK)

set(CMAKE_VERBOSE_MAKEFILE 1)

include_directories(${PROJECT_SOURCE_DIR}/src/libs/avicenna/)
include_directories(${PROJECT_SOURCE_DIR}/src/libs/redis/)
include_directories($ENV{AKELA_SDK_ROOT}/include/)
include_directories(/home/jamil/workspace/boost_1_57_0/)

message("-- linking akela library from ... $ENV{AKELA_SDK_ROOT}/lib")
link_directories($ENV{AKELA_SDK_ROOT}/lib)
message("-- linking avicenna library from ... ${PROJECT_SOURCE_DIR}/src/libs/avicenna")
link_directories(${PROJECT_SOURCE_DIR}/src/libs/avicenna)
message("-- linking redis library from ... ${PROJECT_SOURCE_DIR}/src/libs/redis")
link_directories(${PROJECT_SOURCE_DIR}/src/libs/redis)


add_subdirectory(src)

install(DIRECTORY scripts/
		DESTINATION scripts
		FILES_MATCHING PATTERN "*.sh"
		PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE
    	GROUP_WRITE GROUP_READ GROUP_EXECUTE
 	   	WORLD_WRITE WORLD_READ WORLD_EXECUTE)
install(DIRECTORY configs/
		DESTINATION configs/)
