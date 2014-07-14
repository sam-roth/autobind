

#include "Func.hpp"
#include "../util.hpp"
#include "../CallGenerator.hpp"

namespace autobind {

Func::Func(std::string name)
: Export(name)
, _implRef(gensym(name))
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
	// TODO: implement methods
	out << "PyObject *" << _implRef << "(PyObject *self, PyObject *args, PyObject *kwargs)";
}

void Func::codegenDeclaration(std::ostream &out) const
{
	codegenPrototype(out);
	out << ";\n";
}


void Func::codegenDefinition(std::ostream &out) const
{
	// FIXME: should clear error after failed overload

	codegenPrototype(out);
	out << "\n{\n";
	{
		IndentingOStreambuf indenter(out, "\t");
		for(auto decl : _decls)
		{
			CallGenerator cgen("args", "kwargs", decl);
			cgen.codegen(out);
		}
		out << "return 0;\n";
	}
	out << "}\n";
}


void Func::codegenMethodTable(std::ostream &out) const
{
	// TODO: docstrings
	out << "{"
		<< "\"" << name() << "\", "
		<< "(PyCFunction) &" << _implRef << ", METH_VARARGS | METH_KEYWORDS, "
		<< "0"
		<< "},\n";
}


} // autobind

