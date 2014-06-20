#include "Method.hpp"


namespace autobind {


void Method::codegenCall(std::ostream &out) const
{
	out << "self->object." << name();
	codegenCallArgs(out);
	out << ";\n";
}


} // autobind

