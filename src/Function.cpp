#include <iomanip>
#include <boost/format.hpp>

#include "prefix.hpp"
#include "util.hpp"
#include "stream.hpp"
#include "streamindent.hpp"
#include "printing.hpp"
#include "Function.hpp"
#include "StringTemplate.hpp"

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
, _signatures{std::move(args)}
, _docstring(std::move(docstring))
, _selfTypeName("PyObject")
{
	auto parts = rsplit(this->name(), "::");
	_unqualifiedName = parts.second;
	_implName = gensym(this->name());

}

void Function::codegenCall(std::ostream &out, size_t index) const
{
	if(isMethod())
	{
		out << "self->object.";
	}
	out << name();
	codegenCallArgs(out, index);
	out << ";\n";
}

void Function::codegenCallArgs(std::ostream &out, size_t index) const
{

	using namespace streams;

	auto argStream = stream(signature(index))
		| enumerated()
		| pairTransformed([](int i, const Arg &a) {
			return a.requiresAdditionalConversion?
				  str(boost::format("*arg%1%.value") % i)
				: str(boost::format("arg%1%") % i);
		})
		| interposed(", ");


	out << "(" << cat(argStream) << ")";
}


void Function::codegenDeclaration(std::ostream &out) const
{
	out << boost::format(FunctionPrototype) % _implName % selfTypeName() << ";\n";
}

void Function::codegenTupleUnpack(std::ostream &out, size_t index) const
{
	using namespace streams;

	const auto &sig = signature(index);


	static const StringTemplate t = R"EOF(

	if(!ok)
	{
		PyErr_Clear();
		ok = true;
	}

	static const char *kwlist[] = {
		{{kwlist}}
		0
	};

	{{varDecls}}

	ok = PyArg_ParseTupleAndKeywords(args, kw, "{{format}}", (char **)kwlist{{args}});

	)EOF";


	// call PyArg_ParseTuple
	auto kwlistStream = stream(sig)
		| transformed([](const Arg &a) { return "\"" + a.argName + "\", "; });



	auto argStream = stream(sig)
// 		| take(sig.size())
		| enumerated()
		| pairTransformed([](int i, const Arg &a) -> std::string { 
			if(a.requiresAdditionalConversion)
			{
				std::stringstream ss;
				ss << ", &python::detail::ConversionFunc<";
				ss << a.cppQualTypeName;
				ss << ">::convert";
				ss << ", &arg" << i;
				auto result = ss.str();
				return result;
			}
			else
			{
				return std::string(", &arg") + std::to_string(i);
			}
		});

	auto formatStream = stream(sig)
		| transformed([](const Arg &a) {
			if(a.requiresAdditionalConversion)
			{
				return std::string("O&");
			}
			else
			{
				std::string r;
				r.push_back(a.parseTupleFmt);
				return r;
			}
		});


	t.into(out)
		.setFunc("varDecls", [&](std::ostream &out) {
			// declare variables
			for(const auto &pair : stream(sig) | enumerated())
			{
				if(pair.second.requiresAdditionalConversion)
				{
					out << "python::detail::ConversionFunc<";
					out << pair.second.cppQualTypeName;
					out << ">::Value ";
					out << "arg" << pair.first << ";\n";

					out << "arg" << pair.first << ".errorMessage = \"";
					out << "expected " << pair.second.cppQualTypeName << " for argument ";
					out << pair.first + 1 << " (" << pair.second.argName << ")\";\n";
// 					out << "autobind::Optional<python::ConversionLoadResult<"
// 						<< pair.second.cppQualTypeName
// 						<< ">::type> ";
				}
				else
				{
					out << pair.second.cppQualTypeName << " ";
					out << "arg" << pair.first << ";\n";
				}

				
			}
		})
		.set("kwlist", cat(kwlistStream))
		.set("format", cat(formatStream))
		.set("args",   cat(argStream))
		.expand();

}

void Function::codegenDefinitionBody(std::ostream &out, size_t index) const
{
	// emit "try" block
	out << "try\n{\n";
	{
		IndentingOStreambuf indenter2(out, "\t");
		// call function
		{
// 			if(_lineNo >= 0)
// 				out << boost::format("#line %1% %2%\n") % _lineNo % std::quoted(_origFile);

			if(_returnType != "void")
			{
				out << _returnType << " result = ";
			}

			codegenCall(out, index);
		}


		if(_returnType != "void")
		{
			out << "returnValue = python::Conversion<" << _returnType << ">::dump(result);\n";
		}
		else
		{
			out << "Py_INCREF(Py_None);\nreturnValue = Py_None;\n";
		}
	}
	out << "}\n";
	out << "catch(python::Exception &)\n{\n";
	out << "    returnValue = 0;\n";
	out << "}\n";
	out << "catch(std::exception &exc)\n{\n";
	// emit catch block
	{
		IndentingOStreambuf indenter2(out, "\t");
		out << "returnValue = PyErr_Format(PyExc_RuntimeError, \"%s\", exc.what());\n";
	}

	out << "}\n";
}
void Function::codegenDefinition(std::ostream &out) const
{
	using namespace streams;
	out << boost::format(FunctionPrototype) % _implName % selfTypeName() << "\n{\n";

	{
		IndentingOStreambuf indenter(out, "\t");
		out << "bool ok = true;\nPyObject *returnValue = 0;\n";


		for(size_t i = 0, count = this->signatureCount(); i < count; ++i)
		{
			out << "{\n";
			{
				IndentingOStreambuf indenter(out, "\t");

				codegenTupleUnpack(out, i);
				out << "if(ok)\n{\n";
				{
					IndentingOStreambuf indenter(out, "\t");
					codegenDefinitionBody(out, i);
				}
				out << "}\n";
				out << "if(returnValue) return returnValue;\n";
			}
			out << "}\n";
		}
		out << "return returnValue;\n";
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

void Function::merge(const autobind::Export &e)
{
	if(auto func = dynamic_cast<const Function *>(&e))
	{
		for(const auto &sig : func->signatures())
		{
			_signatures.push_back(sig);
		}
	}
	else
	{
		Export::merge(e); // throws exception
	}
}

}  // namespace autobind


