cmake_minimum_required(VERSION 3.10)

project(mohpc-lib VERSION 1.0)

#file(GLOB_RECURSE SRC_ASSETS "Formats/*.cpp")
#file(GLOB_RECURSE SRC_COLLISION "Collision/*.cpp")
#file(GLOB_RECURSE SRC_COMMON "Common/*.cpp")
#file(GLOB_RECURSE SRC_NETWORK "Network/*.cpp")
#file(GLOB_RECURSE SRC_SCRIPT "Script/*.cpp")
#file(GLOB_RECURSE SRC_MISC "Utilities/*.cpp" "Platform/*.cpp" "Misc/*.cpp" "Managers/*.cpp")
#file(GLOB SRC_MISC "*.cpp" "*.h")
file(GLOB_RECURSE SRC "*.cpp")
list(FILTER SRC EXCLUDE REGEX "Platform/(.*)")

file(GLOB SRC ${SRC} "Platform/*.cpp")
file(GLOB SRC ${SRC} "Platform/Network/*.cpp")

file(GLOB_RECURSE HEADERS "${CMAKE_SOURCE_DIR}/include/*.h")

if(UNIX)
	file(GLOB_RECURSE SRC ${SRC} "Platform/Network/unix/*.cpp")
elseif(WIN32)
	file(GLOB_RECURSE SRC ${SRC} "Platform/Network/win32/*.cpp")
endif()

add_library(MOHPC SHARED ${SRC} "${CMAKE_SOURCE_DIR}/include/MOHPC/mohpc.natvis")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/../thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/../thirdparty/zlib)
include_directories(${PROJECT_SOURCE_DIR}/../thirdparty/GameSpy/src)

# Add support for std c++ filesystem on Unix
if(UNIX)
	target_link_libraries(MOHPC PUBLIC stdc++fs)
elseif(WIN32)
	target_compile_options(MOHPC PRIVATE /W3)
endif()

# Add our third-party library
target_link_libraries(MOHPC PRIVATE MOHPC-3PT)
add_dependencies(MOHPC MOHPC-3PT)

target_compile_definitions(MOHPC PRIVATE MOHPC_DLL=1 FPM_64BIT)
target_include_directories(MOHPC PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

set_property(TARGET MOHPC PROPERTY CMAKE_CXX_STANDARD 17)
set_property(TARGET MOHPC PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET MOHPC PROPERTY CMAKE_CXX_EXTENSIONS OFF)
target_compile_features(MOHPC PUBLIC cxx_std_17)

add_custom_target(versioning ${CMAKE_COMMAND} -D VERSION_FILE=${CMAKE_SOURCE_DIR}/include/MOHPC/Version.h -P ${CMAKE_SOURCE_DIR}/mohpc-lib/version.cmake)
add_dependencies(MOHPC versioning)

install(
	TARGETS MOHPC
	EXPORT MOHPC-Config
	DESTINATION "${CMAKE_INSTALL_PREFIX}"
	INCLUDES DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
)

install(
	DIRECTORY "${CMAKE_SOURCE_DIR}/include/MOHPC"
	DESTINATION include
)

#install(
#	FILES $<TARGET_PDB_FILE:MOHPC>
#	DESTINATION "${CMAKE_INSTALL_PREFIX}"
#)

export(
	TARGETS MOHPC
	NAMESPACE MOHPC::
	FILE "${CMAKE_INSTALL_PREFIX}/mohpc-config.cmake"
)

export(
	PACKAGE MOHPC
)

install(
	EXPORT MOHPC-Config
	DESTINATION "${CMAKE_INSTALL_PREFIX}"
	NAMESPACE MOHPC::
)


