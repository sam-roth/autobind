// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef DATA_HPP_F6Z1XF
#define DATA_HPP_F6Z1XF

#include <memory>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>
#include <unordered_map>
#include <boost/format.hpp>
#include "stream.hpp"
#include "streamindent.hpp"
#include "util.hpp"
namespace autobind {

// std::string gensym(const std::string &prefix="G");


class Module;
class ConversionInfo;

/// Represents the bindings for a C++ declaration.
class Export
{
	std::string _name;
public:
	Export(std::string name)
	: _name(std::move(name)) { }
	virtual ~Export() { }

	const std::string &name() const { return _name; }

	virtual void setModule(Module &) { }

	/// Forward-declare the bindings.
	virtual void codegenDeclaration(std::ostream &) const = 0;
	/// Define the bindings.
	virtual void codegenDefinition(std::ostream &) const = 0;
	/// Emit an entry suitable for inclusion in the PyMethodDef[] table, if applicable.
	virtual void codegenMethodTable(std::ostream &) const = 0;
	/// Emit any code that needs to be run on module init.
	virtual void codegenInit(std::ostream &) const { }

	/// Incorporate the other export into this one. If this is not possible, throw std::runtime_error.
	/// (Do this by calling the superclass implementation.)
	/// This may be used to implement overloading.
	virtual void merge(const Export &other)
	{
		throw std::runtime_error("Cannot merge export " + name() + " with " + other.name());
	}

	virtual bool validate(const ConversionInfo &) const
	{
		return true;
	}
};


} // namespace autobind

#endif // DATA_HPP_F6Z1XF

