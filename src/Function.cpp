#include <iomanip>
#include <boost/format.hpp>

#include "prefix.hpp"
#include "util.hpp"
#include "stream.hpp"
#include "streamindent.hpp"
#include "printing.hpp"
#include "Function.hpp"


#include <regex>

namespace autobind {

namespace
{
	const boost::format FunctionPrototype{
		"static PyObject *%s(%s *self, PyObject *args, PyObject *kw)"
	};


}


std::string processDocString(const std::string &docstring)
{
	auto result = std::regex_replace(docstring, std::regex("(^|\n)\\s*(///|\\*)"), "$1");
	replace(result, "\\", "\\\\");
	replace(result, "\n", "\\n");
	replace(result, "\"", "\\\"");


	return result;
}


Function::Function(std::string name, std::vector<Arg> args,
                   std::string docstring)
: Export(std::move(name))
, _args(std::move(args))
, _docstring(std::move(docstring))
{
	auto parts = rsplit(this->name(), "::");
	_unqualifiedName = parts.second;
	_implName = this->name();
	replace(_implName, "::", "_ns_");
	_implName += "_py_bind_impl";

}

void Function::codegenCall(std::ostream &out) const
{
	out << name();
	codegenCallArgs(out);
	out << ";\n";
}

void Function::codegenCallArgs(std::ostream &out) const
{

	using namespace streams;

	auto argStream = stream(_args)
		| enumerated()
		| pairTransformed([](int i, const Arg &a) {
			return a.requiresAdditionalConversion?
				  str(boost::format("PyConversion<%1%>::load(arg%2%)") % a.cppQualTypeName % i)
				: str(boost::format("arg%1%") % i);
		})
		| interposed(", ");


	out << "(" << cat(argStream) << ")";
}


void Function::codegenDeclaration(std::ostream &out) const
{
	out << boost::format(FunctionPrototype) % _implName % selfTypeName() << ";\n";
}

void Function::codegenTupleUnpack(std::ostream &out) const
{
	using namespace streams;
	// declare variables
	for(const auto &pair : stream(_args) | enumerated())
	{
		if(pair.second.requiresAdditionalConversion)
		{
			out << "PyObject *";
		}
		else
		{
			out << pair.second.cppQualTypeName << " ";
		}

		out << "arg" << pair.first << ";\n";
	}
	
	// call PyArg_ParseTuple
	auto kwlistStream = stream(_args)
		| transformed([](const Arg &a) { return "\"" + a.argName + "\", "; });

	out << boost::format("static const char *kwlist[] = {%1%0};\n") % cat(kwlistStream);

	auto argStream = count()
		| take(_args.size())
		| transformed([](int i) { return boost::format(", &arg%d") % i; });



	auto formatStream = stream(_args)
		| transformed([](const Arg &a) { return a.parseTupleFmt; });


	out << boost::format("if(!PyArg_ParseTupleAndKeywords(args, kw, \"%s\", (char **)kwlist%s))\n"
	                     "    return 0;\n") % cat(formatStream) % cat(argStream);

}

void Function::codegenDefinitionBody(std::ostream &out) const
{
	// emit "try" block
	out << "try\n{\n";
	{
		IndentingOStreambuf indenter2(out);
		// call function
		{
			if(_lineNo >= 0)
				out << boost::format("#line %1% %2%\n") % _lineNo % std::quoted(_origFile);

			if(_returnType != "void")
			{
				out << _returnType << " result = ";
			}

			codegenCall(out);
		}


		if(_returnType != "void")
		{
			out << "return PyConversion<" << _returnType << ">::dump(result);\n";
		}
		else
		{
			out << "Py_RETURN_NONE;\n";
		}
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
void Function::codegenDefinition(std::ostream &out) const
{
	using namespace streams;
	out << boost::format(FunctionPrototype) % _implName % selfTypeName() << "\n{\n";

	{
		IndentingOStreambuf indenter(out);
		codegenTupleUnpack(out);
		codegenDefinitionBody(out);
	}

	out << "}\n";
}

void Function::codegenMethodTable(std::ostream &out) const
{
	auto docstring = processDocString(_docstring);

	out << boost::format("{\"%1%\", (PyCFunction) %2%, METH_VARARGS|METH_KEYWORDS, %3%},\n") 
		% pythonName()
		% implName()
		% ("\"" + docstring + "\"");
}
}  // namespace autobind


