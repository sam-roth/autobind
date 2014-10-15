// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include <clang/AST/DeclCXX.h>

#include "Func.hpp"
#include "../util.hpp"
#include "../printing.hpp"
#include "../CallGenerator.hpp"
#include "../StringTemplate.hpp"
#include "../ClassData.hpp"
#include "../diagnostics.hpp"
#include "../DiscoveryVisitor.hpp"

namespace autobind {

Func::Func(std::string name)
: Export(name)
, _implRef(gensym(name))
, _selfTypeRef("PyObject")
{
}


void Func::merge(const autobind::Export &other) 
{
	if(auto that = dynamic_cast<const autobind::Func *>(&other))
	{
		for(auto decl : that->decls())
		{
			addDecl(*decl);
		}
	}
	else
	{
		Export::merge(other);
	}
}
void Func::codegenPrototype(std::ostream &out) const
{
	out << "PyObject *" << _implRef << "(" << _selfTypeRef << " *self, PyObject *args, PyObject *kwargs)";
}

void Func::codegenDeclaration(std::ostream &out) const
{
	codegenPrototype(out);
	out << ";\n";
}

void Func::codegenOverload(std::ostream &out, size_t n) const
{
	auto &decl = decls().at(n);
	const char *prefix = _selfTypeRef == "PyObject"? "" : "self->object.";

	CallGenerator cgen("args", "kwargs", decl, prefix);
	cgen.codegen(out);
}


void Func::codegenDefinition(std::ostream &out) const
{
	codegenPrototype(out);
	out << "\n{\n";
	{
		IndentingOStreambuf indenter(out, "\t");
		beforeOverloads(out);
		for(size_t i = 0; i < _decls.size(); ++i)
		{
			codegenOverload(out, i);
		}
		out << "return 0;\n";
	}
	out << "}\n";
}

std::string Func::docstringEscaped() const
{
	std::string docstringEscaped;
	for(auto decl : _decls)
	{
		if(!docstringEscaped.empty())
		{
			docstringEscaped += "\\n";
		}
	
		docstringEscaped += createPythonSignature(*decl);
		docstringEscaped += "\\n";
	
		auto thisOverloadDocEscaped = processDocString(findDocumentationComments(*decl));
		if(!thisOverloadDocEscaped.empty())
		{
			docstringEscaped += thisOverloadDocEscaped;
			docstringEscaped += "\\n";
		}
	}

	return docstringEscaped;
}

void Func::codegenMethodTable(std::ostream &out) const
{
	out << "{"
		<< "\"" << name() << "\", "
		<< "(PyCFunction) &" << _implRef << ", METH_VARARGS | METH_KEYWORDS, "
		<< "\"" << docstringEscaped() << "\""
		<< "},\n";
}

Constructor::Constructor(const autobind::ClassData &classData)
: Export(classData.exportName())
, Func(classData.exportName())
, ClassExport(classData.exportName(), classData)
{
	setSelfTypeRef(classData.wrapperRef());
}

void Constructor::codegenPrototype(std::ostream &out) const
{
	out << "PyObject *" << implRef() << "(PyTypeObject *ty, PyObject *args, PyObject *kwargs)";
}

void Constructor::codegenOverload(std::ostream &out, size_t n) const
{
	codegenOverloadOrDefault(out, n);
}


void Constructor::codegenOverloadOrDefault(std::ostream &out, int n) const
{

	auto decl = n >= 0? llvm::dyn_cast_or_null<clang::CXXConstructorDecl>(decls().at(n)) : nullptr;

	static const StringTemplate top = R"EOF(
	try
	{
		PyErr_Clear();
		self->initialized = false;
		{{unpackTuple}}
		if({{unpackOk}})
		{
			new((void *) &self->object) {{wrappedType}}({{callArgs}});
			self->initialized = true;
			return (PyObject *)self;
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
	)EOF";

	TupleUnpacker unpacker("args", "kwargs");
	if(decl)
	{
		for(auto param : streams::stream(decl->param_begin(), decl->param_end()))
		{
			unpacker.addElement(*param);
		}
	}

	top.into(out)
		.setFunc("unpackTuple", method(unpacker, &TupleUnpacker::codegen))
		.set("unpackOk", unpacker.okRef())
		.set("wrappedType", classData().typeRef())
		.set("callArgs", streams::cat(streams::stream(unpacker.elementRefs()).interpose(", ")))
		.expand();
}

void Constructor::beforeOverloads(std::ostream &out) const
{
	static const StringTemplate allocSelf = R"EOF(
	{{structName}} *self = ({{structName}} *) ty->tp_alloc(ty, 0);
	if(!self) return 0;
	)EOF";

	allocSelf.into(out)
		.set("structName", selfTypeRef())
		.expand();

	if(classData().isDefaultConstructible())
	{
		codegenOverloadOrDefault(out, -1);
	}
}

bool Func::validate(const autobind::ConversionInfo &info) const
{
	using namespace streams;
	bool result = true;
	for(auto func : _decls)
	{
		for(auto param : stream(func->param_begin(),
		                        func->param_end()))
		{
			result = info.ensureConversionSpecializationExists(param, param->getType().getTypePtr()) && result;
		}

		auto resultTy = func->getReturnType();
		if(!resultTy->isVoidType())
		{
			result = info.ensureConversionSpecializationExists(func, resultTy.getTypePtr()) && result;
		}
	}

	return result;
}

Descriptor::Descriptor(const std::string &name, 
                       const autobind::ClassData &classData)
: Export(name)
, ClassExport(name, classData)
{ }


void Descriptor::merge(const autobind::Export &exp)
{
	if(auto that = dynamic_cast<const Descriptor *>(&exp))
	{
		if(that->_getter) setGetter(that->_getter);
		if(that->_setter) setSetter(that->_setter);
	}
	else
	{
		ClassExport::merge(exp);
	}
}
void Descriptor::setSetter(const clang::FunctionDecl *value)
{
	_setter = value;
	if(_setter)
		_setterRef = gensym(classData().wrapperRef() + "_" + _setter->getNameAsString());
	else
		_setterRef = "0";
}


void Descriptor::setGetter(const clang::FunctionDecl *value) 
{
	_getter = value;
	if(_getter)
		_getterRef = gensym(classData().wrapperRef() + "_" + _getter->getNameAsString());
	else
		_getterRef = "0";
}

void Descriptor::codegenDeclaration(std::ostream &out) const
{
	if(_getter)
	{
		static const StringTemplate tpl = R"EOF(
		static PyObject *{{implName}}({{selfTypeName}} *self, void */*closure*/);
		)EOF";

		tpl.into(out)
			.set("implName", _getterRef)
			.set("selfTypeName", classData().wrapperRef())
			.expand();
	}

	if(_setter)
	{
		static const StringTemplate tpl = R"EOF(
		static int {{implName}}({{selfTypeName}} *self, PyObject *value, void *closure);
		)EOF";
		
		tpl.into(out)
			.set("implName", _setterRef)
			.set("selfTypeName", classData().wrapperRef())
			.expand();
	}
}



void Descriptor::codegenDefinition(std::ostream &out) const
{
	if(_getter)
	{
		static const StringTemplate tpl = R"EOF(		
		static PyObject *{{implName}}({{selfTypeName}} *self, void */*closure*/)
		{
			try
			{
				PyObject *result = ::python::Conversion<{{type}}>::dump(self->object.{{func}}());
				PyErr_Clear();
				return result;
			}
			catch(::python::Exception &exc)
			{
				return 0;
			}
			catch(::std::exception &exc)
			{
				PyErr_SetString(PyExc_RuntimeError, exc.what());
				return 0;
			}
		}
		)EOF";

		if(_getter->param_size() != 0)
			diag::stop(**_getter->param_begin(), "getter must have no parameters");

		if(_getter->getReturnType()->isVoidType())
			diag::stop(*_getter, "getter must not return `void`.");
			
		auto ty = _getter->getReturnType().getNonReferenceType();
		ty.removeLocalConst();
		ty.removeLocalRestrict();
		ty.removeLocalVolatile();

		tpl.into(out)
			.set("implName", _getterRef)
			.set("selfTypeName", classData().wrapperRef())
			.set("type", ty.getCanonicalType().getAsString())
			.set("func", _getter->getQualifiedNameAsString())
			.expand();
	}

	if(_setter)
	{
		static const StringTemplate tpl = R"EOF(
		static int {{implName}}({{selfTypeName}} *self, PyObject *value, void *closure)
		{
			if(!value)
			{
				PyErr_SetString(PyExc_TypeError, "Cannot delete attribute.");
				return -1;
			}

			try
			{
				self->object.{{func}}(::python::Conversion<{{type}}>::load(value));
				PyErr_Clear();
				return 0;
			}
			catch(::python::Exception &exc)
			{
				return -1;
			}
			catch(::std::exception &exc)
			{
				PyErr_SetString(PyExc_RuntimeError, exc.what());
				return -1;
			}
		}
		)EOF";

		if(_setter->param_size() != 1)
			diag::stop(*_setter, "setter must have exactly one parameter");

		auto ty = (**_setter->param_begin()).getType().getNonReferenceType();
		ty.removeLocalConst();
		ty.removeLocalRestrict();
		ty.removeLocalVolatile();

		tpl.into(out)
			.set("implName", _setterRef)
			.set("selfTypeName", classData().wrapperRef())
			.set("type", ty.getCanonicalType().getAsString())
			.set("func", _setter->getNameAsString())
			.expand();
	}
}


void Descriptor::codegenMethodTable(std::ostream &) const
{
	// intentionally left blank
}

std::string Descriptor::escapedDocstring() const
{
	std::string result;
	if(_getter)
	{
		result += findDocumentationComments(*_getter);
	}

	if(_setter)
	{
		if(_getter) result += "\n\n";

		result += findDocumentationComments(*_setter);
	}

	return processDocString(result);
}


void Descriptor::codegenGetSet(std::ostream &out) const
{
	out << "{(char *)\"" << name() << "\", "
		<< "(getter) " << _getterRef << ", "
		<< "(setter) " << _setterRef << ", "
		<< "(char *)\"" << escapedDocstring() << "\"},\n";
}

Method::Method(const std::string &name, const autobind::ClassData &classData)
: Export(name)
, Func(name)
, ClassExport(name, classData)
{
	setSelfTypeRef(classData.wrapperRef());
}




} // autobind

