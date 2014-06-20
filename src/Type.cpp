
#include "Type.hpp"
#include "Method.hpp"
#include "util.hpp"
#include "StringTemplate.hpp"

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

void Type::addMethod(std::unique_ptr<Method> method)
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
	out << "struct " << _structName << "\n{\n";
	{
		IndentingOStreambuf indenter(out);

		out << "PyObject_HEAD\n";
		out << _cppQualTypeName << " object;\n";

	}
	out << "};\n";


	out << "static PyObject *" << _structName << "_new(PyTypeObject *ty, PyObject *args, PyObject *kw);\n";
	out << "static int " << _structName << "_init(" << _structName << " *self, PyObject *args, PyObject *kw);\n";
	out << "static void " << _structName << "_dealloc(" << _structName << " *self);\n";

	for(const auto &method : _methods) method->codegenDeclaration(out);

}

void Type::codegenDefinition(std::ostream &out) const 
{

	out << "static PyObject *" << _structName << "_new(PyTypeObject *ty, PyObject *args, PyObject *kw)\n{\n";
	{
		IndentingOStreambuf indenter(out);
		out << _structName << " *self = (" << _structName << " *)ty->tp_alloc(ty, 0);\n";
		out << "if(!self) return 0;\n";
		
		// emit "try" block
		out << "try\n{\n";
		{
			IndentingOStreambuf indenter2(out);
			// call function
		
			out << "new((void *)&self->object) " << _cppQualTypeName;
			constructor().codegenCallArgs(out);
			out << ";\n";
			out << "return (PyObject *) self;\n";
		}
		out << "}\n";
		out << "catch(python::Exception &)\n{\n";
		out << "    return 0;\n";
		out << "}\n";
		out << "catch(std::exception &exc)\n{\n";
		// emit catch block
		{
			IndentingOStreambuf indenter2(out);
			out << "return PyErr_Format(PyExc_RuntimeError, \"%s\", exc.what());\n";
		}

		out << "}\n";

		
	
	}
	out << "}\n";

	out << "static int " << _structName << "_init(" << _structName << " *self, PyObject *args, PyObject *kw)\n{\n";
	{
		IndentingOStreambuf indenter(out);
		out << "return 0;\n";
	}
	out << "}\n";

	out << "static void " << _structName << "_dealloc(" << _structName << " *self)\n{\n";
	{
		IndentingOStreambuf indenter(out);

		auto destructor = "~" + rsplit(_cppQualTypeName, "::").second;

		out << "self->object." << destructor << "();\n";
		out << "Py_TYPE(self)->tp_free((PyObject *)self);\n";
	}
	out << "}\n";

	for(const auto &method : _methods) method->codegenDefinition(out);

	out << "static PyMethodDef " << _structName << "_methods[] = {\n";

	{
		IndentingOStreambuf indenter(out);
		for(const auto &method : _methods)
		{
			method->codegenMethodTable(out);
		}

		out << "{0, 0, 0, 0}\n";
	}

	out << "};\n";


	std::string reprName = "0";
	std::string strName = "0";

	for(const auto &method : _methods)
	{
		if(method->operatorName() == "repr" || method->operatorName() == "str")
		{
			out << boost::format("static PyObject *%1%_%2%(%1% *self)\n{\n")
				% _structName
				% method->operatorName();

			{
				IndentingOStreambuf indenter(out);
				method->codegenDefinitionBody(out);
			}

			out << "}\n";

			if(method->operatorName() == "repr")
			{
				reprName = _structName + "_repr";
			}
			else
			{
				strName = _structName + "_str";
			}
		}

	}

	const char *typeObjectFormatString = R"EOF(
static PyTypeObject {{structName}}_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)                     
	"{{moduleName}}.{{name}}",                 /* tp_name */
	sizeof({{structName}}),               /* tp_basicsize */
    0,                         /* tp_itemsize */       
	(destructor){{structName}}_dealloc,   /* tp_dealloc */
    0,                         /* tp_print */          
    0,                         /* tp_getattr */        
    0,                         /* tp_setattr */        
    0,                         /* tp_reserved */       
	python::protocols::detail::ReprConverter<{{cppName}}, {{structName}}>::get(),             /* tp_repr */
    0,                         /* tp_as_number */      
    0,                         /* tp_as_sequence */    
    0,                         /* tp_as_mapping */     
    0,                         /* tp_hash  */          
    0,                         /* tp_call */           
	python::protocols::detail::StrConverter<{{cppName}}, {{structName}}>::get(),             /* tp_str */
    0,                         /* tp_getattro */       
    0,                         /* tp_setattro */       
    0,                         /* tp_as_buffer */      
    Py_TPFLAGS_DEFAULT |                               
        Py_TPFLAGS_BASETYPE,   /* tp_flags */          
    "",                        /* tp_doc */            
    0,                         /* tp_traverse */       
    0,                         /* tp_clear */          
    0,                         /* tp_richcompare */    
    0,                         /* tp_weaklistoffset */ 
    0,                         /* tp_iter */           
    0,                         /* tp_iternext */       
	{{structName}}_methods,               /* tp_methods */
    0,                         /* tp_members */        
    0,                         /* tp_getset */         
    0,                         /* tp_base */           
    0,                         /* tp_dict */           
    0,                         /* tp_descr_get */      
    0,                         /* tp_descr_set */      
    0,                         /* tp_dictoffset */     
	(initproc){{structName}}_init,        /* tp_init */
    0,                         /* tp_alloc */          
	{{structName}}_new,                   /* tp_new */
};
	)EOF";

	SimpleTemplateNamespace names;
	names
		.set("structName", _structName)
		.set("moduleName", _moduleName)
		.set("name", name())
		.set("reprName", reprName)
		.set("strName", strName)
		.set("cppName", _cppQualTypeName);

	StringTemplate(typeObjectFormatString).expand(out, names);

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

