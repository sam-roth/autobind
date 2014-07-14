#include "CallGenerator.hpp"
#include "stream.hpp"
#include "printing.hpp"
#include "StringTemplate.hpp"

namespace autobind {
	
CallGenerator::CallGenerator(std::string argsRef, 
                             std::string kwargsRef,
                             const clang::FunctionDecl *decl)
: _unpacker(std::move(argsRef), std::move(kwargsRef))
, _decl(decl)
{
	for(auto param : streams::stream(decl->param_begin(), decl->param_end()))
	{
		_unpacker.addElement(*param);
	}
}


void CallGenerator::codegen(std::ostream &out) const
{
	// TODO: implement return values
	// TODO: implement methods

	static const StringTemplate top = R"EOF(
	{{unpack}}

	if({{ok}})
	{
		try
		{
			{{prefix}}{{name}}(
				{{args}}
			);
		}
		catch(::python::Exception &exc)
		{
			{{pythonException}}
		}
		catch(::std::exception &exc)
		{
			{{stdException}}
		}

		{{success}}

	}
	)EOF";

	top.into(out)
		.setFunc("unpack", method(_unpacker, &TupleUnpacker::codegen))
		.set("ok", _unpacker.okRef())
		.set("prefix", "")
		.set("name", _decl->getNameAsString())
		.set("args", streams::cat(streams::stream(_unpacker.elementRefs()) 
		                          | streams::interposed(",\n")))
		.setFunc("pythonException", method(*this, &CallGenerator::codegenPythonException))
		.setFunc("stdException", method(*this, &CallGenerator::codegenStdException))
		.setFunc("success", method(*this, &CallGenerator::codegenSuccess))
		.expand();



}

void CallGenerator::codegenPythonException(std::ostream &out) const
{
	codegenErrorReturn(out);
}


void CallGenerator::codegenStdException(std::ostream &out) const
{
	// TODO: allow for customizing exception type
	out << "PyErr_SetString(PyExc_RuntimeError, exc.what());\n";
	codegenErrorReturn(out);
}

void CallGenerator::codegenErrorReturn(std::ostream &out) const
{
	out << "return 0;\n";
}

void CallGenerator::codegenSuccess(std::ostream &out) const
{
	out << "PyErr_Clear();\nPy_RETURN_NONE;\n";
}


} // autobind


