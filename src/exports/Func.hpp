// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef FUNC_HPP_2Q8A6A
#define FUNC_HPP_2Q8A6A

#include <vector>
#include <clang/AST/Decl.h>

#include "../Export.hpp"
#include "ClassExport.hpp"

namespace autobind {

class Func: public virtual Export
{
	std::vector<const clang::FunctionDecl *> _decls;
	std::string _implRef;
	std::string _selfTypeRef;
protected:
	virtual void codegenPrototype(std::ostream &) const;
	virtual void codegenOverload(std::ostream &, size_t) const;
	virtual void beforeOverloads(std::ostream &) const { }
public:
	Func(std::string name);

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;

	void addDecl(const clang::FunctionDecl &decl) 
	{
		_decls.push_back(&decl);
	}

	const std::vector<const clang::FunctionDecl *> &decls() const
	{
		return _decls;
	}

	void setSelfTypeRef(std::string r)
	{
		_selfTypeRef = std::move(r);
	}

	const std::string &selfTypeRef() const
	{
		return _selfTypeRef;
	}

	const std::string &implRef() const
	{
		return _implRef;
	}

	virtual void merge(const Export &e) override;
	virtual ~Func() { }
};


// TODO: move this to another file
class Constructor: public Func, public ClassExport
{
	void codegenOverloadOrDefault(std::ostream &, int) const;
public:
	Constructor(const ClassData &classData);

protected:
	virtual void codegenPrototype(std::ostream &) const;
	virtual void codegenOverload(std::ostream &, size_t) const;
	virtual void beforeOverloads(std::ostream &) const;
};


/// ClassExport for getters and setters
class Descriptor: public ClassExport
{
	const clang::FunctionDecl *_setter = nullptr, *_getter = nullptr;
	std::string _getterRef="0", _setterRef="0";
	std::string escapedDocstring() const;

public:
	Descriptor(const std::string &name,
	           const ClassData &classData);

	void setSetter(const clang::FunctionDecl *value);
	void setGetter(const clang::FunctionDecl *value);

	virtual void merge(const Export &e) override;

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;
	virtual void codegenGetSet(std::ostream &) const override;
};


class Method: public Func, public ClassExport
{
public:
	Method(const std::string &name,
	       const ClassData &classData);
};


} // autobind

#endif // FUNC_HPP_2Q8A6A

