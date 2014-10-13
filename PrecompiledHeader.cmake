# Macro for setting up precompiled headers. Usage:
#
#	add_precompiled_header(target header.h [FORCEINCLUDE])
#
# MSVC: A source file with the same name as the header must exist and
# be included in the target (E.g. header.cpp).
#
# MSVC: Add FORCEINCLUDE to automatically include the precompiled
# header file from every source file.
#
# GCC: The precompiled header is always automatically included from
# every header file.
#
# Copyright (C) 2009-2013 Lars Christensen <larsch@belunktum.dk>
# Copyright (C) 2014 Sam Roth <sam.roth1@gmail.com>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

function(add_precompiled_header target_name input_file)
	get_filename_component(_inputWe ${input_file} NAME_WE)
	set(pch_source ${_inputWe}.cpp)
	foreach(arg ${ARGN})
		if(arg STREQUAL FORCEINCLUDE)
			set(FORCEINCLUDE ON)
		else(arg STREQUAL FORCEINCLUDE)
			set(FORCEINCLUDE OFF)
		endif(arg STREQUAL FORCEINCLUDE)
	endforeach(arg)

	if(MSVC)
		get_target_property(sources ${target_name} SOURCES)
		set(_sourceFound FALSE)
		foreach(_source ${sources})
			set(PCH_COMPILE_FLAGS "")
			if(_source MATCHES \\.\(cc|cxx|cpp\)$)
		get_filename_component(_sourceWe ${_source} NAME_WE)
		if(_sourceWe STREQUAL ${_inputWe})
			set(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /Yc${input_file}")
			set(_sourceFound TRUE)
		else(_sourceWe STREQUAL ${_inputWe})
			set(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /Yu${input_file}")
			if(FORCEINCLUDE)
				set(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /FI${input_file}")
			endif(FORCEINCLUDE)
		endif(_sourceWe STREQUAL ${_inputWe})
		set_source_files_properties(${_source} PROPERTIES COMPILE_FLAGS "${PCH_COMPILE_FLAGS}")
			endif(_source MATCHES \\.\(cc|cxx|cpp\)$)
		endforeach()
		if(NOT _sourceFound)
			message(FATAL_ERROR "A source file for ${input_file} was not found. Required for MSVC builds.")
		endif(NOT _sourceFound)
	endif(MSVC)
		

	if(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
		get_filename_component(_name ${input_file} NAME)
		get_filename_component(srcfiledir ${input_file} DIRECTORY)
		set(_source "${CMAKE_CURRENT_SOURCE_DIR}/${input_file}")
		set(_output "${CMAKE_CURRENT_SOURCE_DIR}/${srcfiledir}/${_name}.gch")

		string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
		set(_compiler_FLAGS ${${_flags_var_name}})

		get_directory_property(_directory_flags INCLUDE_DIRECTORIES)
		foreach(item ${_directory_flags})
			list(APPEND _compiler_FLAGS "-I${item}")
		endforeach(item)

		get_directory_property(_directory_flags DEFINITIONS)
		list(APPEND _compiler_FLAGS ${_directory_flags})

		foreach(item ${CMAKE_CXX_FLAGS})
			LIST(APPEND _compiler_FLAGS ${item})
		endforeach()

		set(compiler_command 
			"${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -x c++-header -o ${_output} ${_source} ${PCH_ADDL_FLAGS}"
		)

		separate_arguments(compiler_command)

		message("${compiler_command}")

		#message("${CMAKE_CXX_COMPILER} -DPCHCOMPILE ${_compiler_FLAGS} -x c++-header -o {_output} ${_source} ${PCH_ADDL_FLAGS}")
		separate_arguments(_compiler_FLAGS)
		add_custom_command(
			OUTPUT ${_output}
			COMMAND ${compiler_command}
			DEPENDS ${_source} 
		)
		add_custom_target(${target_name}_gch DEPENDS ${_output})
		add_dependencies(${target_name} ${target_name}_gch)

		set_target_properties(${target_name} PROPERTIES COMPILE_FLAGS "-include ${CMAKE_CURRENT_SOURCE_DIR}/${input_file} -Winvalid-pch")
	 	get_target_property(_targetFlags ${target_name} COMPILE_FLAGS)
		message("target properties: ${_targetFlags}")
	endif()
endfunction()
