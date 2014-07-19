// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef TUPLEUNPACKER_HPP_8BN4HT
#define TUPLEUNPACKER_HPP_8BN4HT
#include <string>
#include <clang/AST/Decl.h>
#include "util.hpp"
namespace autobind {

/// Generates code for unpacking a Python argument tuple.
class TupleUnpacker
{
	struct StorageElt
	{
		std::string type, name;
		std::string msg;
		std::string realType;
	};

	const std::string _argsRef, _kwargsRef;
	const std::string _okRef;
	std::string _format;
	std::vector<StorageElt> _storageElements;
	std::vector<std::string> _elementRefs;
	std::vector<std::string> _argNames;
public:
	/// Initialize the TupleUnpacker given the names of the variables containing
	/// args and kwargs.
	TupleUnpacker(std::string argsRef,
	              std::string kwargsRef)
	: _argsRef(std::move(argsRef))
	, _kwargsRef(std::move(kwargsRef))
	, _okRef(gensym("ok"))
	{ }


	/// Add a variable declaration to the list of values to be unpacked from the tuple.
	/// (The type and name of the variable declaration will be used.)
	void addElement(const clang::VarDecl &decl);

	/// Get expressions that will refer to the unpacked elements of the tuple.
	const std::vector<std::string> &elementRefs() const;

	/// Get the name of the boolean flag used to indicate whether unpacking was successful.
	const std::string &okRef() const;

	/// Generate the code for the tuple unpack.
	void codegen(std::ostream &) const;
};


} // autobind



#endif // TUPLEUNPACKER_HPP_8BN4HT

