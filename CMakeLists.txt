
cmake_minimum_required(VERSION 2.6)
project(autobind)

include(PrecompiledHeader.cmake)

# -fvisibility-inlines-hidden: fixes linker warnings about symbol visibility when linking with Clang
# -Wno-deprecated-register: suppress warnings caused by Boost.Regex


set(STDLIB_FLAG "-stdlib=libc++")
set(CMAKE_CXX_FLAGS "-g -std=c++11 -O0 -Wall -D__STDC_CONSTANT_MACROS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_LIMIT_MACROS -fvisibility-inlines-hidden -Wno-deprecated-register")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "lib")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

include(CMakeLists.user.cmake OPTIONAL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STDLIB_FLAG}")

find_package(Boost)
find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)


link_directories(${LLVM_LIB_DIR})

include_directories(
	${Boost_INCLUDE_DIR}
	${CLANG_INCLUDE_DIRS}
	${LLVM_INCLUDE_DIR}
)


add_library(
	autobind_core
	SHARED

	src/Function.cpp
	src/DiscoveryVisitor.cpp
	src/Type.cpp
	src/Getter.cpp
	src/util.cpp
	src/Module.cpp
)

target_link_libraries(
	autobind_core
# 	${Boost_LIBRARIES}
	${CLANG_LIBS}
	${LLVM_LIBS_CORE}
)

ADD_PRECOMPILED_HEADER(autobind_core src/prefix.hpp)

add_executable(autobind src/main.cpp)
target_link_libraries(autobind autobind_core)

add_executable(test src/test.cpp)
target_link_libraries(test autobind_core)

add_library(autobind_plugin SHARED src/plugin.cpp)
target_link_libraries(autobind_plugin autobind_core)

