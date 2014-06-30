
#include "Type.hpp"
#include "Method.hpp"
#include "util.hpp"
#include "StringTemplate.hpp"
#include "Module.hpp"

namespace autobind {

Type::Type(std::string name,
           std::string cppQualTypeName,
           std::string docstring)
: Export(std::move(name))
, _cppQualTypeName(std::move(cppQualTypeName))
, _docstring(std::move(docstring))
{
	_structName = gensym(this->name());
}

void Type::addMethod(std::unique_ptr<Function> method)
{
	method->setSelfTypeName(_structName);
	_methods.push_back(std::move(method));
}

Function &Type::constructor() const
{
	if(!_constructor)
	{
		_constructor = std::unique_ptr<Function>(new Function("", {}, ""));
	}

	return *_constructor;
}

void Type::setConstructor(std::unique_ptr<Function> func)
{
	_constructor = std::move(func);
}


void Type::codegenDeclaration(std::ostream &out) const
{


	static const StringTemplate declTemplate = R"EOF(
	class {{typeName}};
	struct {{structName}};

	static PyObject *{{structName}}_new(PyTypeObject *ty, PyObject *args, PyObject *kw);
	static int       {{structName}}_init({{structName}} *self, PyObject *args, PyObject *kw);
	static void      {{structName}}_dealloc({{structName}} *self);
	)EOF";

	declTemplate.into(out)
		.set("structName", _structName)
		.set("typeName", _cppQualTypeName)
		.expand();


	for(const auto &method : _methods) method->codegenDeclaration(out);

	if(_copyAvailable)
	{
		static const StringTemplate tpl = R"EOF(
		template <>
		struct python::Conversion<{{typeName}}>
		{
			static PyObject *dump(const {{typeName}} &obj);
			static {{typeName}} &load(PyObject *obj);

		};
		)EOF";

		tpl.into(out)
			.set("typeName", _cppQualTypeName)
			.expand();
	}
}

void Type::codegenDefinition(std::ostream &out) const 
{


	static const StringTemplate newFunc = R"EOF(

	struct {{structName}}
	{
		PyObject_HEAD
		{{typeName}} object;
	};
	
	static PyObject *{{structName}}_new(PyTypeObject *ty, PyObject *args, PyObject *kw)
	{
		{{structName}} *self = ({{structName}} *)ty->tp_alloc(ty, 0);
		if(!self) return 0;

		int ok = 1;

		try
		{
			{{unpackTuple}}
			if(ok)
			{
				new((void *) &self->object) {{typeName}}{{callArgs}};
				return (PyObject *)self;
			}
			else
			{
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
	)EOF";

	newFunc.into(out)
		.set("structName", _structName)
		.setFunc("unpackTuple", [&](std::ostream &out) {
			constructor().codegenTupleUnpack(out, 0);
		})
		.set("typeName", _cppQualTypeName)
		.setFunc("callArgs", [&](std::ostream &out) {
			constructor().codegenCallArgs(out, 0);
		})
		.expand();


	static const StringTemplate initDeallocTemplate = R"EOF(

	static int {{structName}}_init({{structName}} *self, PyObject *args, PyObject *kw)
	{
		return 0;
	}

	static void {{structName}}_dealloc({{structName}} *self)
	{
		self->object.{{destructor}}();
		Py_TYPE(self)->tp_free((PyObject *)self);
	}

	)EOF";

	initDeallocTemplate.into(out)
		.set("structName", _structName)
		.set("destructor", "~" + rsplit(_cppQualTypeName, "::").second)
		.expand();


	for(const auto &method : _methods) method->codegenDefinition(out);


	static const StringTemplate methodsTemplate = R"EOF(

	static PyMethodDef {{structName}}_methods[] = {
		{{methodTable}}
		{0, 0, 0, 0}
	};

	static PyGetSetDef {{structName}}_getset[] = {
		{{getSetTable}}
		{0}
	};

	)EOF";

	std::map<std::string, Setter *> setters;
	for(const auto &m : _methods)
	{
		if(auto setter = dynamic_cast<Setter *>(m.get()))
		{
			setters[setter->pythonName()] = setter;
		}
	}
	methodsTemplate.into(out)
		.set("structName", _structName)
		.setFunc("methodTable", [&](std::ostream &out) {
			for(const auto &method : _methods)
			{
				method->codegenMethodTable(out);
			}
		})
		.setFunc("getSetTable", [&](std::ostream &out) {
			for(auto getter : getters())
			{
				std::string setterName = "0";
				auto it = setters.find(getter->pythonName());

				if(it != setters.end())
				{
					setterName = it->second->implName();
				}

				out << boost::format("{(char *)\"%1%\", (getter)%2%, (setter)%4%, (char *)\"%3%\", 0},\n")
					% getter->pythonName() % getter->implName() % processDocString(getter->docstring())
					% setterName;
			}
		})
		.expand();

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
		{{structName}}_new,                                                           /* tp_new */
	};
	)EOF";


	typeObjectTemplate.into(out)
		.set("structName", _structName)
		.set("moduleName", _moduleName)
		.set("name", name())
		.set("cppName", _cppQualTypeName)
		.set("typeName", _cppQualTypeName)
		.set("docstring", processDocString(_docstring))
		.expand();


	if(_copyAvailable)
	{
		static const StringTemplate conversionImplTemplate = R"EOF(
		PyObject * python::Conversion<{{typeName}}>::dump(const {{typeName}} &obj)
		{
			PyTypeObject *ty = &{{structName}}_Type;

			{{structName}} *self = ({{structName}} *)ty->tp_alloc(ty, 0);

			try
			{
				new ((void *) &self->object) {{typeName}}(obj);
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
			.set("structName", _structName)
			.set("moduleName", _moduleName)
			.set("name", name())
			.set("cppName", _cppQualTypeName)
			.set("typeName", _cppQualTypeName)
			.expand();


	}


}

void Type::codegenMethodTable(std::ostream &) const
{

}

void Type::setModule(Module &m)
{
	_moduleName = m.name();
}

void Type::codegenInit(std::ostream &out) const
{
	out << boost::format("if(PyType_Ready(&%1%_Type) < 0) return 0;\n") % _structName;
	out << boost::format("Py_INCREF(&%1%_Type);\n") % _structName;
	out << boost::format("PyModule_AddObject(mod, \"%1%\", (PyObject *) &%2%_Type);\n") % name() % _structName;
}

} // autobind

