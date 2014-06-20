
#include "Getter.hpp"


namespace autobind {

namespace
{
	const boost::format FunctionPrototype{
		"static PyObject *%s(%s *self, void *)"
	};


}
void Getter::codegenDeclaration(std::ostream &out) const
{
	out << boost::format(FunctionPrototype) % implName() % selfTypeName() << ";\n";
}

void Getter::codegenDefinition(std::ostream &out) const
{
	out << boost::format(FunctionPrototype) % implName() % selfTypeName() << "\n{\n";

	{
		IndentingOStreambuf indenter(out);
		codegenDefinitionBody(out);
	}

	out << "}\n";
}


} // autobind



