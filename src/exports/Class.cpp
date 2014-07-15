// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclBase.h>

#include "Func.hpp"
#include "Class.hpp"
#include "../printing.hpp"
#include "../StringTemplate.hpp"
#include "../TupleUnpacker.hpp"

namespace autobind {

Class::Class(const clang::CXXRecordDecl &decl)
: Export(decl.getNameAsString())
, _decl(decl)
, _selfTypeRef(gensym(decl.getNameAsString()))
{
	std::unordered_map<std::string, std::unique_ptr<Func>> functions;

	for(auto it = decl.method_begin(), end = decl.method_end(); it != end; ++it)
	{
		auto kind = it->getKind();
		if(kind == clang::CXXMethodDecl::CXXConstructor)
		{
			// TODO: constructor overloading
			auto constructor = llvm::dyn_cast_or_null<clang::CXXConstructorDecl>(*it);
			if(!_constructor && !constructor->isCopyConstructor() && !constructor->isMoveConstructor())
			{
				_constructor = constructor;
			}
		}
		else if(kind == clang::CXXMethodDecl::CXXDestructor)
		{
			// TODO:
		}
		else if(!it->isStatic() && it->getAccess() == clang::AS_public)
		{
			auto name = it->getNameAsString();
			auto &decl = functions[name];
			if(!decl)
			{
				decl.reset(new Func(name));
			}

			decl->addDecl(**it);
			decl->setSelfTypeRef(_selfTypeRef);
		}
	}

	for(auto &pair : functions)
	{
		_functions.push_back(std::move(pair.second));
	}
}


void Class::codegenDeclaration(std::ostream &out) const
{
	static const StringTemplate tpl = R"EOF(
	struct {{selfTypeRef}};

	static PyObject *{{selfTypeRef}}_new(PyTypeObject *ty, PyObject *args, PyObject *kw);
	static int       {{selfTypeRef}}_init({{selfTypeRef}} *self, PyObject *args, PyObject *kw);
	static void      {{selfTypeRef}}_dealloc({{selfTypeRef}} *self);
	)EOF";

	// TODO: copy constructors

	tpl.into(out)
		.set("selfTypeRef", _selfTypeRef)
		.set("wrappedType", _decl.getQualifiedNameAsString())
		.expand();

	for(const auto &func : _functions)
	{
		func->codegenDeclaration(out);
	}
}


void Class::codegenDefinition(std::ostream &out) const
{
	static const StringTemplate tpl = R"EOF(

	struct {{selfTypeRef}}
	{
		PyObject_HEAD
		bool initialized;
		{{wrappedType}} object;
	};

	static PyObject *{{selfTypeRef}}_new(PyTypeObject *ty, PyObject *args, PyObject *kw)
	{
		{{selfTypeRef}} *self = ({{selfTypeRef}} *)ty->tp_alloc(ty, 0);
		if(!self) return 0;

		self->initialized = false;

		int ok = 1;

		try
		{
			{{unpackTuple}}
			if({{unpackOk}})
			{
				new((void *) &self->object) {{wrappedType}}{{callArgs}};
				self->initialized = true;
				return (PyObject *)self;
			}
			else
			{
				Py_XDECREF(self);
				return 0;
			}
		}
		catch(python::Exception &)
		{
			Py_XDECREF(self);
			return 0;
		}
		catch(std::exception &exc)
		{
			Py_XDECREF(self);
			return PyErr_Format(PyExc_RuntimeError, "%s", exc.what());
		}
	}

	static int {{selfTypeRef}}_init({{selfTypeRef}} *self, PyObject *args, PyObject *kw)
	{
		return 0;
	}

	static void {{selfTypeRef}}_dealloc({{selfTypeRef}} *self)
	{
		if(self->initialized)
			self->object.{{destructor}}();
		Py_TYPE(self)->tp_free((PyObject *)self);
	}

	)EOF";


	TupleUnpacker unpacker("args", "kwargs");
	if(_constructor)
	{
		for(auto arg : streams::stream(_constructor->param_begin(),
		                               _constructor->param_end()))
		{
			unpacker.addElement(*arg);
		}
	}


	auto wrappedTypeName = _decl.getQualifiedNameAsString();

	tpl.into(out)
		.set("wrappedType", wrappedTypeName)
		.set("selfTypeRef", _selfTypeRef)
		.setFunc("unpackTuple", method(unpacker, &TupleUnpacker::codegen))
		.set("callArgs", streams::cat(streams::stream(unpacker.elementRefs())
		                              | streams::interposed(", ")))
		.set("unpackOk", unpacker.okRef())
		.set("destructor", "~" + wrappedTypeName) 
		.expand();

	for(const auto &func : _functions)
	{
		func->codegenDefinition(out);
	}
}


void Class::codegenMethodTable(std::ostream &) const
{
	// TODO: implement
}


void Class::merge(const autobind::Export &other) 
{
	return Export::merge(other);
}


} // autobind

