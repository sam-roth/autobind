#include "Method.hpp"


namespace autobind {

// 	
// void Method::codegenCall(std::ostream &out) const
// {
// 	out << "self->object." << name();
// 	codegenCallArgs(out);
// 	out << ";\n";
// }
// 
// 
// void Method::codegenMethodTable(std::ostream &out) const
// {
// 	auto docstring = processDocString(this->docstring());
//
// 	out << boost::format("{\"%1%\", (PyCFunction) %2%, METH_VARARGS, %3%},\n") 
// 		% unqualifiedName()
// 		% implName()
// 		% ("\"" + docstring + "\"");
// }
// 
} // autobind

