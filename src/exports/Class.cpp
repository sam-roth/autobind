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
#include "../attributeStream.hpp"
#include "../diagnostics.hpp"

namespace autobind {
	
Class::Class(const clang::CXXRecordDecl &decl)
: Export(decl.getNameAsString())
, _decl(decl)
, _classData(decl)
, _constructor(_classData)
{
	_selfTypeRef = _classData.wrapperRef();

	std::unordered_map<std::string, std::unique_ptr<Func>> functions;
	using namespace streams;
	_constructor.setSelfTypeRef(_selfTypeRef);
	for(auto it = decl.method_begin(), end = decl.method_end(); it != end; ++it)
	{
		auto kind = it->getKind();
		if(kind == clang::CXXMethodDecl::CXXConstructor)
		{
			auto constructor = llvm::dyn_cast_or_null<clang::CXXConstructorDecl>(*it);

			if(!constructor->isCopyConstructor() && !constructor->isMoveConstructor())
			{
				_constructor.addDecl(*constructor);
			}
		}
		else if(kind == clang::CXXMethodDecl::CXXDestructor)
		{
			// TODO:
		}
		else if(!it->isStatic() 
		        && it->getAccess() == clang::AS_public 
		        && !it->isOverloadedOperator())
		{
			auto name = it->getNameAsString();

			bool omit = false;

			for(auto attr : attributeStream(**it))
			{
				auto annot = attr->getAnnotation();
				if(annot.startswith("pygetter:"))
				{
					Descriptor d(annot.split(':').second, classData());
					d.setGetter(*it);
					mergeDescriptor(d);
					omit = true;
					break;
				}
				else if(annot.startswith("pysetter:"))
				{
					Descriptor d(annot.split(':').second, classData());
					d.setSetter(*it);
					mergeDescriptor(d);
					omit = true;
					break;
				}
			}

			if(!omit)
			{
				auto &decl = functions[name];
				if(!decl)
				{
					decl.reset(new Func(name));
				}
				decl->addDecl(**it);
				decl->setSelfTypeRef(_selfTypeRef);
			}
		}

	}

	for(auto &pair : functions)
	{
		_functions.push_back(std::move(pair.second));
	}
}

void Class::mergeDescriptor(const autobind::Descriptor &d)
{
	auto &existing = _descriptors[d.name()];
	if(!existing)
	{
		existing.reset(new Descriptor(d.name(), classData()));
	}

	existing->merge(d);
}


void Class::codegenDeclaration(std::ostream &out) const
{
	static const StringTemplate tpl = R"EOF(
	class {{wrappedType}};
	struct {{selfTypeRef}};

	static PyObject *{{selfTypeRef}}_new(PyTypeObject *ty, PyObject *args, PyObject *kw);
	static int       {{selfTypeRef}}_init({{selfTypeRef}} *self, PyObject *args, PyObject *kw);
	static void      {{selfTypeRef}}_dealloc({{selfTypeRef}} *self);
	)EOF";



	tpl.into(out)
		.set("selfTypeRef", _selfTypeRef)
		.set("wrappedType", _decl.getQualifiedNameAsString())
		.expand();

	for(const auto &func : _functions)
	{
		func->codegenDeclaration(out);
	}

	for(const auto &desc : _descriptors)
	{
		desc.second->codegenDeclaration(out);
	}

	// TODO: handle noncopyable types
	static const StringTemplate converterTemplate = R"EOF(
	template <>
	struct python::Conversion<{{typeName}}>
	{
		static PyObject *dump(const {{typeName}} &obj);
		static {{typeName}} &load(PyObject *obj);

	};
	)EOF";

	converterTemplate.into(out)
		.set("typeName", _decl.getQualifiedNameAsString())
		.expand();
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

	static PyMethodDef {{selfTypeRef}}_methods[] = {
		{{methodTable}}
		{0, 0, 0, 0}
	};

	static PyGetSetDef {{selfTypeRef}}_getset[] = {
		{{getSetTable}}
		{0}
	};

	)EOF";

	auto wrappedTypeName = _decl.getQualifiedNameAsString();

	tpl.into(out)
		.set("wrappedType", wrappedTypeName)
		.set("selfTypeRef", _selfTypeRef)
		.set("destructor", "~" + wrappedTypeName)
		.setFunc("methodTable", [&](std::ostream &out) {
			for(const auto &func : _functions)
			{
				func->codegenMethodTable(out);
			}
		})
		.setFunc("getSetTable", [&](std::ostream &out) { 
			for(const auto &pair : _descriptors)
			{
				pair.second->codegenGetSet(out);
			}
		})
		.expand();

	for(const auto &func : _functions)
	{
		func->codegenDefinition(out);
	}

	_constructor.codegenDefinition(out);


	static const StringTemplate typeObjectTemplate = R"EOF(
	static PyTypeObject {{structName}}_Type = {
		PyVarObject_HEAD_INIT(NULL, 0)                     
		"{{moduleName}}.{{name}}",                                                    /* tp_name */
		sizeof({{structName}}),                                                       /* tp_basicsize */
		0,                                                                            /* tp_itemsize */       
		(destructor){{structName}}_dealloc,                                           /* tp_dealloc */
		0,                                                                            /* tp_print */          
		0,                                                                            /* tp_getattr */        
		0,                                                                            /* tp_setattr */        
		0,                                                                            /* tp_reserved */       
		python::protocols::detail::ReprConverter<{{cppName}}, {{structName}}>::get(), /* tp_repr */
		0,                                                                            /* tp_as_number */      
		0,                                                                            /* tp_as_sequence */    
		0,                                                                            /* tp_as_mapping */     
		0,                                                                            /* tp_hash  */          
		0,                                                                            /* tp_call */           
		python::protocols::detail::StrConverter<{{cppName}}, {{structName}}>::get(),  /* tp_str */
		PyObject_GenericGetAttr,                                                      /* tp_getattro */       
		PyObject_GenericSetAttr,                                                      /* tp_setattro */       
		python::protocols::detail::BufferProcs<{{cppName}}, {{structName}}>::get(),   /* tp_as_buffer */      
		Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                                     /* tp_flags */          
		"{{docstring}}",                                                              /* tp_doc */
		0,                                                                            /* tp_traverse */       
		0,                                                                            /* tp_clear */          
		0,                                                                            /* tp_richcompare */    
		0,                                                                            /* tp_weaklistoffset */ 
		0,                                                                            /* tp_iter */           
		0,                                                                            /* tp_iternext */       
		{{structName}}_methods,                                                       /* tp_methods */
		0,                                                                            /* tp_members */        
		{{structName}}_getset,                                                        /* tp_getset */
		0,                                                                            /* tp_base */           
		0,                                                                            /* tp_dict */           
		0,                                                                            /* tp_descr_get */      
		0,                                                                            /* tp_descr_set */      
		0,                                                                            /* tp_dictoffset */     
		(initproc){{structName}}_init,                                                /* tp_init */
		0,                                                                            /* tp_alloc */          
		{{constructorRef}},                                                           /* tp_new */
	};
	)EOF";


	typeObjectTemplate.into(out)
		.set("structName", _selfTypeRef)
		.set("moduleName", _moduleName)
		.set("name", name())
		.set("cppName", _decl.getQualifiedNameAsString())
		.set("typeName", _decl.getQualifiedNameAsString())
		.set("docstring", processDocString(findDocumentationComments(_decl)))
		.set("constructorRef", _constructor.implRef())
		.expand();

	for(const auto &pair : _descriptors)
	{
		pair.second->codegenDefinition(out);
	}
	// TODO: handle noncopyables

	static const StringTemplate conversionImplTemplate = R"EOF(
		PyObject * python::Conversion<{{typeName}}>::dump(const {{typeName}} &obj)
		{
			PyTypeObject *ty = &{{structName}}_Type;

			{{structName}} *self = ({{structName}} *)ty->tp_alloc(ty, 0);

			try
			{
				new ((void *) &self->object) {{typeName}}(obj);
				self->initialized = true;
				return (PyObject *)self;
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

		{{typeName}} &python::Conversion<{{typeName}}>::load(PyObject *obj)
		{
			int rv = PyObject_IsInstance(obj, (PyObject *) &{{structName}}_Type);
			if(rv < 0)
			{
				throw python::Exception();
			}

			if(rv == 1)
			{
				return (({{structName}} *) obj)->object;
			}
			else
			{
				throw std::runtime_error("Expected an instance of {{typeName}}.");
			}
		}
	)EOF";


	conversionImplTemplate.into(out)
		.set("structName", _selfTypeRef)
		.set("moduleName", _moduleName)
		.set("name", name())
		.set("cppName", _decl.getQualifiedNameAsString())
		.set("typeName", _decl.getQualifiedNameAsString())
		.expand();

}


void Class::codegenMethodTable(std::ostream &) const
{
}


void Class::merge(const autobind::Export &other) 
{
	return Export::merge(other);
}


void Class::codegenInit(std::ostream &out) const
{
	static const StringTemplate tpl = R"EOF(
	if(PyType_Ready(&{{selfTypeRef}}_Type) < 0) return 0;
	Py_INCREF(&{{selfTypeRef}}_Type);
	PyModule_AddObject(mod, "{{name}}", (PyObject *) &{{selfTypeRef}}_Type);
	)EOF";

	tpl.into(out)
		.set("selfTypeRef", _selfTypeRef)
		.set("name", name())
		.expand();
}
} // autobind

