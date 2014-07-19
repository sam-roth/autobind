
#include <clang/AST/DeclCXX.h>

#include "Func.hpp"
#include "../util.hpp"
#include "../printing.hpp"
#include "../CallGenerator.hpp"
#include "../StringTemplate.hpp"


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


void Func::codegenMethodTable(std::ostream &out) const
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

	out << "{"
		<< "\"" << name() << "\", "
		<< "(PyCFunction) &" << _implRef << ", METH_VARARGS | METH_KEYWORDS, "
		<< "\"" << docstringEscaped << "\""
		<< "},\n";
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
		.set("wrappedType", name())
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

	if(_defaultConstructible)
	{
		codegenOverloadOrDefault(out, -1);
	}
}



} // autobind

