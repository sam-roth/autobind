
cmake_minimum_required(VERSION 2.6)
project(autobind)

include(PrecompiledHeader.cmake)
include(CMakeParseArguments)
# include(Autobind.cmake)

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
find_package(Curses REQUIRED)


link_directories(
	${LLVM_LIB_DIR}
)

include_directories(
	${Boost_INCLUDE_DIR}
	${CLANG_INCLUDE_DIRS}
	${LLVM_INCLUDE_DIR}
	${CURSES_INCLUDE_DIR}
)


macro(ab_add_clang_plugin name)
	set(_sourcefiles ${ARGN})
# 	list(REMOVE_AT _sourcefiles 1)
	add_library(${name} MODULE ${_sourcefiles})
	set_target_properties(
		${name} PROPERTIES
		LINK_FLAGS "-Wl,-flat_namespace -Wl,-undefined -Wl,suppress"
	)
endmacro()

# message("CLANG_LIBS ${CLANG_LIBS}")

# add_library(
# 	ab_libs
# 	INTERFACE
# )
# 
# target_link_libraries(
# 	ab_libs
# 	INTERFACE
# 	${EXTRA_LIBS}
# # 	${Boost_LIBRARIES}
# 	${CLANG_LIBS}
# 	${LLVM_LIBS_CORE}
# )
# 
add_library(
	autobind_core
	SHARED

	src/exports/Func.cpp
	src/exports/Class.cpp
	src/ClassData.cpp
	src/CallGenerator.cpp
	src/DiscoveryVisitor.cpp
	src/util.cpp
	src/Module.cpp
	src/TupleUnpacker.cpp
	src/diagnostics.cpp
)


target_link_libraries(
	autobind_core
# 
	${EXTRA_LIBS}
	${Boost_LIBRARIES}
	${CLANG_LIBS}
	${LLVM_LIBS_CORE}
	${CURSES_LIBRARIES}
)

ADD_PRECOMPILED_HEADER(autobind_core src/prefix.hpp)

add_executable(autobind src/main.cpp)
target_link_libraries(autobind autobind_core)

add_executable(test src/test.cpp)
target_link_libraries(test autobind_core)

# add_library(autobind_plugin SHARED src/plugin.cpp)

ab_add_clang_plugin(autobind_plugin src/plugin.cpp)
target_link_libraries(autobind_plugin autobind_core)

# set_target_properties(autobind_plugin PROPERTIES
# 	LINK_FLAGS "-Wl,-flat_namespace -Wl,-undefined -Wl,suppress")

# ab_add_module(example2 examples/example2.cpp)

# ab_outputname(example2gen examples/example2.cpp)
# add_library(example2 SHARED ${example2gen})
# ab_generate(example2 examples/example2.cpp)
# ab_get_compile_flags(compflags autobind_core)
# message("compiler flags ab_core: ${compflags}")

