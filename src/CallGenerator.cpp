#include "CallGenerator.hpp"
#include "stream.hpp"
#include "printing.hpp"
#include "StringTemplate.hpp"

namespace autobind {

CallGenerator::CallGenerator(std::string argsRef, 
                             std::string kwargsRef,
                             const clang::FunctionDecl *decl,
                             std::string prefix)
: _unpacker(std::move(argsRef), std::move(kwargsRef))
, _decl(decl)
, _prefix(std::move(prefix))
{
	for(auto param : streams::stream(decl->param_begin(), decl->param_end()))
	{
		_unpacker.addElement(*param);
	}
}


void CallGenerator::codegen(std::ostream &out) const
{
	static const StringTemplate top = R"EOF(
	{{unpack}}
	
	if({{ok}})
	{
		try
		{
			{{resultDecl}}{{prefix}}{{name}}(
				{{args}}
			);

			{{success}}
		}
		catch(::autobind::Exception &exc)
		{
			{{pythonException}}
		}
		catch(::std::exception &exc)
		{
			{{stdException}}
		}
	}
	)EOF";

	std::string resultDecl;
	if(!_decl->getReturnType()->isVoidType())
	{
		resultDecl = _decl->getReturnType().getAsString() + " result = ";
	}

	top.into(out)
		.setFunc("unpack", method(_unpacker, &TupleUnpacker::codegen))
		.set("ok", _unpacker.okRef())
		.set("prefix", _prefix)
		.set("resultDecl", resultDecl)
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
	out << "PyErr_Clear();\n";

	auto ty = _decl->getReturnType().getNonReferenceType();
	ty.removeLocalConst();
	ty.removeLocalVolatile();
	ty.removeLocalRestrict();
	
	if(_decl->getReturnType()->isVoidType())
	{
		out << "Py_RETURN_NONE;\n";
	}
	else
	{
		out << "return ::autobind::Conversion<"
			<< ty.getAsString()
			<< ">::dump(result);";
	}
}


} // autobind


