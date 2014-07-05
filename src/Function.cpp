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
	auto result = regex::regex_replace(docstring, regex::regex("(^|\n+)\\s*(///|\\*)"), std::string("$1"));
	result = regex::regex_replace(result, regex::regex("(^|\n+)\\s+"), std::string("$1"));
	replace(result, "\\", "\\\\");
	replace(result, "\n", "\\n");
	replace(result, "\"", "\\\"");

	

	return result;
}



Function::Function(std::string name, std::vector<Arg> args,
                   std::string docstring)
: Export(std::move(name))
, _docstring(std::move(docstring))
, _selfTypeName("PyObject")
{
	Signature s;
	s.args = std::move(args);
	_signatures.push_back(std::move(s));

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

	auto argStream = stream(signature(index).args)
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
	auto kwlistStream = stream(sig.args)
		| transformed([](const Arg &a) { return "\"" + a.argName + "\", "; });



	auto argStream = stream(sig.args)
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

	auto formatStream = stream(sig.args)
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
			for(const auto &pair : stream(sig.args) | enumerated())
			{
				if(pair.second.requiresAdditionalConversion)
				{
					out << "python::detail::ConversionFunc<";
					out << pair.second.cppQualTypeName;
					out << ">::Value ";
					out << "arg" << pair.first << ";\n";

					out << "arg" << pair.first << ".errorMessage = \"";
					out << "expected object convertible to " << pair.second.typeNameSpelling << " for argument ";
					out << pair.first + 1 << " (" << pair.second.argName << ")\";\n";
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

	auto &sig = signature(index);

	out << "try\n{\n";
	{
		IndentingOStreambuf indenter2(out, "\t");
		// call function
		if(sig.cppReturnTypeName != "void")
		{
			out << sig.cppReturnTypeName << " result = ";
		}

		codegenCall(out, index);

		out << "PyErr_Clear();\n";
		if(sig.cppReturnTypeName != "void")
		{
			out << "returnValue = python::Conversion<" << sig.cppReturnTypeName << ">::dump(result);\n";
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

void Function::codegenArgSizeCalc(std::ostream &out) const
{
	out << "PyTuple_GET_SIZE(args) + (kw? PyDict_Size(kw) : 0)";
}

void Function::codegenDefinition(std::ostream &out) const
{
	using namespace streams;

	static const StringTemplate root = R"EOF(
	{{prototype}}
	{
		bool ok = true;
		PyObject *returnValue = 0;

		const size_t argCount = {{argSize}};

		switch(argCount)
		{
			{{cases}}
			default: {
				PyErr_Format(PyExc_TypeError,
				             "Unexpected number of arguments to function: %d.",
				             argCount);

				return 0;
			}
		}

		return returnValue;
	}
	)EOF";

	static const StringTemplate case_ = R"EOF(
	case {{count}}: {
		{{overloads}}
	} break;
	)EOF";

	static const StringTemplate overload = R"EOF(
	{
		{{unpack}}
		if(ok)
		{
			{{body}}
			return returnValue;
		}
	}
	)EOF";

	root.into(out)
		.set("prototype", boost::format(FunctionPrototype) % _implName % selfTypeName())
		.setFunc("argSize", [&](std::ostream &out) {
			codegenArgSizeCalc(out);
		})
		.setFunc("cases", [&](std::ostream &out) {
			// group overloads by argument count
			std::vector<std::pair<size_t, size_t>> overloadRanges;
			overloadRanges.emplace_back(0, 0);
			size_t prevSize = 0;
			for(const auto &sig : stream(_signatures) | enumerated())
			{
				if(prevSize != sig.second.args.size())
				{
					overloadRanges.emplace_back(sig.first, sig.first + 1);
					prevSize = sig.second.args.size();
				}
				else
				{
					overloadRanges.back().second = sig.first + 1;
				}
			}
			out << "\n// found " << overloadRanges.size() << " overload ranges\n";

			for(auto range : overloadRanges)
			{
				if(range.first == range.second) continue;

				case_.into(out)
					.set("count", _signatures[range.first].args.size())
					.setFunc("overloads", [&](std::ostream &out) {
						for(size_t i = range.first; i < range.second; ++i)
						{
							overload.into(out)
								.setFunc("unpack", [&](std::ostream &out) { codegenTupleUnpack(out, i); })
								.setFunc("body", [&](std::ostream &out) { codegenDefinitionBody(out, i); })
								.expand();
						}
					})
					.expand();
			}
		})
		.expand();


#if 0
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
					out << "return returnValue;\n";
				}
				out << "}\n";
// 				out << "if(returnValue) return returnValue;\n";
			}
			out << "}\n";
		}
		out << "return returnValue;\n";
	}
	out << "}\n";
#endif
}

void Function::codegenMethodTable(std::ostream &out) const
{
	std::stringstream docstringStream;
	using namespace streams;
	for(const auto &sig : _signatures)
	{
		auto argStream = stream(sig.args)
			| transformed([](const Arg &arg) { return arg.argName + ": " + arg.typeNameSpelling; })
			| interposed(", ");
			
		docstringStream << "(" << cat(argStream) << ") -> " << sig.returnTypeSpelling << "\n";
	}

	docstringStream << "\n" << _docstring;

	auto docstring = processDocString(docstringStream.str());


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

		// sort signatures by length
		std::stable_sort(_signatures.begin(),
		                 _signatures.end(),
		                 [](const Signature &a, const Signature &b) { return a.args.size() < b.args.size(); });


		if(!func->docstring().empty())
		{
			_docstring += "\n\n" + func->docstring();
		}
	}
	else
	{
		Export::merge(e); // throws exception
	}
}

}  // namespace autobind


