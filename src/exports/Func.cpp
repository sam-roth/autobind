

#include "Func.hpp"
#include "../util.hpp"
#include "../CallGenerator.hpp"

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


void Func::codegenDefinition(std::ostream &out) const
{
	// FIXME: should clear error after failed overload

	const char *prefix = _selfTypeRef == "PyObject"? "" : "self->object.";
	
	codegenPrototype(out);
	out << "\n{\n";
	{
		IndentingOStreambuf indenter(out, "\t");
		for(auto decl : _decls)
		{
			CallGenerator cgen("args", "kwargs", decl, prefix);
			cgen.codegen(out);
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

	// TODO: docstrings
	out << "{"
		<< "\"" << name() << "\", "
		<< "(PyCFunction) &" << _implRef << ", METH_VARARGS | METH_KEYWORDS, "
		<< "\"" << docstringEscaped << "\""
		<< "},\n";
}


} // autobind

