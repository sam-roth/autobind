
#include "type.hpp"
#include "util.hpp"

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

	// bad idea is bad
// 
// 	out << "template <>\nstruct PyConversion<" << _cppQualTypeName << " *>\n{\n";
// 	{
// 		IndentingOStreambuf indenter(out);
// 
// 
// 		out << "static PyObject *dump(" << _cppQualTypeName << "* x)\n{\n";
// 		out << "    return reinterpret_cast<PyObject *>(reinterpret_cast<char *>(x) - offsetof(" << _structName << ", object));\n";
// 		out << "}\n";
// 
//
// 	}
// 	out << "};\n";
// 
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

	out << boost::format(
		"static PyTypeObject %1%_Type = {                      \n"
		"    PyVarObject_HEAD_INIT(NULL, 0)                     \n"
		"    \"%2%.%3%\",             /* tp_name */           \n"
		"    sizeof(%1%),             /* tp_basicsize */      \n"
		"    0,                         /* tp_itemsize */       \n"
		"    (destructor)%1%_dealloc, /* tp_dealloc */        \n"
		"    0,                         /* tp_print */          \n"
		"    0,                         /* tp_getattr */        \n"
		"    0,                         /* tp_setattr */        \n"
		"    0,                         /* tp_reserved */       \n"
		"    0,                         /* tp_repr */           \n"
		"    0,                         /* tp_as_number */      \n"
		"    0,                         /* tp_as_sequence */    \n"
		"    0,                         /* tp_as_mapping */     \n"
		"    0,                         /* tp_hash  */          \n"
		"    0,                         /* tp_call */           \n"
		"    0,                         /* tp_str */            \n"
		"    0,                         /* tp_getattro */       \n"
		"    0,                         /* tp_setattro */       \n"
		"    0,                         /* tp_as_buffer */      \n"
		"    Py_TPFLAGS_DEFAULT |                               \n"
		"        Py_TPFLAGS_BASETYPE,   /* tp_flags */          \n"
		"    \"\",           /* tp_doc */            \n"
		"    0,                         /* tp_traverse */       \n"
		"    0,                         /* tp_clear */          \n"
		"    0,                         /* tp_richcompare */    \n"
		"    0,                         /* tp_weaklistoffset */ \n"
		"    0,                         /* tp_iter */           \n"
		"    0,                         /* tp_iternext */       \n"
		"    0,             /* tp_methods */        \n"  // TODO:
		"    0,             /* tp_members */        \n"  // TODO:
		"    0,                         /* tp_getset */         \n"
		"    0,                         /* tp_base */           \n"
		"    0,                         /* tp_dict */           \n"
		"    0,                         /* tp_descr_get */      \n"
		"    0,                         /* tp_descr_set */      \n"
		"    0,                         /* tp_dictoffset */     \n"
		"    (initproc)%1%_init,      /* tp_init */           \n"
		"    0,                         /* tp_alloc */          \n"
		"    %1%_new,                 /* tp_new */            \n"
		"};                                                     \n"
		"                                                       \n")
		% _structName % _moduleName % name();



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

