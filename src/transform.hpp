
#pragma once
#include <map>
#include <string>
#include <vector>

#include <clang/AST/Type.h>
#include <clang/AST/ASTContext.h>
#include "util.hpp"
#include <boost/format.hpp>
#include "streamindent.hpp"
#include "stream.hpp"
#include "printing.hpp"
#include "data.hpp"

namespace autobind {

struct ParseTupleSnippet: public Streamable<ParseTupleSnippet>
{
	std::vector<std::string> argtypes;
	std::string format;




	void write(std::ostream &os) const
	{

		using namespace streams;


		auto decls = stream(argtypes)
			| enumerated()
			| pairTransformed([&](int i, const std::string &ty) {
				return boost::format("%1% arg%2%;\n") % ty % i;
			});

		os << cat(decls);

		auto argPointers = count()
			| take(argtypes.size())
			| transformed([&](int i) { return ", &arg" + str(i); });

		os << boost::format("if(!PyArg_ParseTuple(args, \"%1%\"%2%))\n"
		                    "    return 0;\n") % format % cat(argPointers);
	}

};

struct CallAndAdaptSnippet: public Streamable<CallAndAdaptSnippet>
{
	std::string functionName;
	std::vector<std::string> adaptDestTypes;

	void write(std::ostream &os) const
	{
		using namespace streams;

		auto funcArgs = stream(adaptDestTypes)
			| enumerated()
			| pairTransformed([&](int i, const std::string &ty) {
				return ty.empty()?
					  (boost::format("arg%1%") % i).str()
					: (boost::format("PyConversion<%1%>::load(arg%2%)") % ty % i).str();
			})
			| interposed(", ");

		os << functionName << "(" << cat(funcArgs) << ");\n";
	}
};

struct BindingSnippet: public Streamable<BindingSnippet>
{
	std::string returnTypeName;
	ParseTupleSnippet parseTupleSnippet;
	CallAndAdaptSnippet callAndAdaptSnippet;

	void write(std::ostream &os) const
	{
		os << boost::format("static extern \"C\" PyObject *\n"
		                    "%1%_impl(PyObject *self, PyObject *args)\n"
		                    "{\n") % callAndAdaptSnippet.functionName;
		os << indented(parseTupleSnippet);
		os << indented(boost::format("%1% result = %2%\n") % returnTypeName % callAndAdaptSnippet);
		os << "}\n";
	}
};

struct WrapperEmitter
{
	clang::ASTContext *ctx;

	std::map<const clang::Type *, std::string> _format;

	WrapperEmitter(clang::ASTContext *ctx)
	: ctx(ctx)
	{
		auto constCharStarTy = ctx->getPointerType(ctx->getConstType(ctx->CharTy));
		_format[ctx->IntTy.getTypePtr()] = "i";
		_format[constCharStarTy.getTypePtr()] = "s";
	}


	std::unique_ptr<autobind::Function> function(clang::FunctionDecl *decl)
	{
		std::vector<autobind::Function::Arg> args;

		for(auto param : streams::stream(decl->param_begin(),
		                                 decl->param_end()))
		{

			autobind::Function::Arg a;

			a.argName = param->getName();
			a.requiresAdditionalConversion = _format.count(param->getType().getTypePtr()) == 0;
			a.cppQualTypeName = param->getType().getUnqualifiedType().getNonReferenceType().getUnqualifiedType().getAsString();
			auto fmt = get(_format, param->getType().getTypePtr()).get_value_or("O");

			assert(!fmt.empty());

			a.parseTupleFmt = fmt.front();

			args.push_back(std::move(a));
		}

		std::string docstring;

		if(auto comment = decl->getASTContext().getRawCommentForAnyRedecl(decl))
		{
			if(comment->isDocumentation())
			{
				docstring = comment->getRawText(decl->getASTContext().getSourceManager());
			}
		}
		auto result = std::unique_ptr<autobind::Function>(new autobind::Function(decl->getQualifiedNameAsString(),
		                                                                         std::move(args),
		                                                                         std::move(docstring)));
		auto sr = decl->getSourceRange();

		auto &sm = decl->getASTContext().getSourceManager();
		auto firstLineNum = sm.getPresumedLineNumber(sr.getBegin());
		auto fileName = sm.getFilename(sr.getEnd());

		result->setSourceLocation(firstLineNum, fileName);

		result->setReturnType(decl->getResultType().getUnqualifiedType().getNonReferenceType().getUnqualifiedType().getAsString());

		return result;

	}

	BindingSnippet snippet(llvm::ArrayRef<clang::ParmVarDecl *> params)
	{

		BindingSnippet result;

		for(auto param : params)
		{
			if(auto format = get(_format, param->getType().getTypePtr()))
			{
				result.parseTupleSnippet.format += *format;
				result.parseTupleSnippet.argtypes.push_back(param->getType().getAsString());
				result.callAndAdaptSnippet.adaptDestTypes.push_back("");
			}
			else
			{
				auto ty = param->getType();
				
				auto nrty = ty.getUnqualifiedType().getNonReferenceType();
// 				nrty.removeLocalCVRQualifiers(clang::Qualifiers::CVRMask);

				result.parseTupleSnippet.format += "O"; // other object type
				result.parseTupleSnippet.argtypes.push_back("PyObject *");
				result.callAndAdaptSnippet.adaptDestTypes.push_back(nrty.getAsString());
			}
		}

		return result;
	}

};

} // namespace autobind

