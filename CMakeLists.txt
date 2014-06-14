
cmake_minimum_required(VERSION 2.6)
project(hgr)

include(PrecompiledHeader.cmake)

set(LLVM_CONFIG_EXECUTABLE "/opt/local/bin/llvm-config-mp-3.3")
set(CMAKE_CXX_FLAGS "-g -std=c++1y -stdlib=libc++ -O3 -Wall -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "lib")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})
find_package(Boost COMPONENTS system regex)
find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)


link_directories(${LLVM_LIB_DIR})
message("CLANG LIBS: ${CLANG_LIBS}")

include_directories(
	${Boost_INCLUDE_DIR}
	${CLANG_INCLUDE_DIRS}
	${LLVM_INCLUDE_DIR}
)


add_executable(
	hgr
	src/main.cpp
	src/function.cpp
	src/DiscoveryVisitor.cpp
)

target_link_libraries(
	hgr
	${Boost_LIBRARIES}
	${CLANG_LIBS}
	${QT_LIBRARIES}
	${LLVM_LIBS_CORE}
)

ADD_PRECOMPILED_HEADER(hgr src/prefix.hpp)

