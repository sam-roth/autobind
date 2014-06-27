#ifndef DATAEXTRACTOR_HPP_OPB0V8
#define DATAEXTRACTOR_HPP_OPB0V8


#include <map>
#include <string>
#include <vector>

#include <clang/AST/Type.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>

#include "util.hpp"
#include "streamindent.hpp"
#include "stream.hpp"
#include "printing.hpp"
#include "Export.hpp"
#include "Method.hpp"
#include "Getter.hpp"

namespace autobind {


inline clang::QualType removeRefsAndCVR(const clang::QualType &ty)
{
	return ty.getCanonicalType().getNonReferenceType().getUnqualifiedType();
}


struct DataExtractor
{
	clang::ASTContext *ctx;

	std::map<const clang::Type *, std::string> _format;

	DataExtractor(clang::ASTContext *ctx)
	: ctx(ctx)
	{
		auto constCharStarTy = ctx->getPointerType(ctx->getConstType(ctx->CharTy));
		_format[ctx->IntTy.getTypePtr()] = "i";
		_format[constCharStarTy.getTypePtr()] = "s";
	}



	template <class T>
	std::unique_ptr<T> genericFunction(clang::FunctionDecl *decl, const std::string &forceName="")
	{
		std::vector<Function::Arg> args;

		for(auto param : streams::stream(decl->param_begin(),
		                                 decl->param_end()))
		{

			// initialize Arg structure
			autobind::Function::Arg a;
			{
				a.argName = param->getName();
				a.requiresAdditionalConversion = _format.count(param->getType().getTypePtr()) == 0;
				a.cppQualTypeName = removeRefsAndCVR(param->getType()).getAsString();
				auto fmt = get(_format, param->getType().getTypePtr()).get_value_or("O");
				assert(!fmt.empty());
				a.parseTupleFmt = fmt.front();
			}


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



		auto result = std::unique_ptr<T>(new T(decl->getQualifiedNameAsString(),
		                                       std::move(args),
		                                       std::move(docstring)));

		result->setPythonName(forceName);
		
		auto sr = decl->getSourceRange();

		auto &sm = decl->getASTContext().getSourceManager();
		auto firstLineNum = sm.getPresumedLineNumber(sr.getBegin());
		auto fileName = sm.getFilename(sr.getEnd());

		result->setSourceLocation(firstLineNum, fileName);
		result->setReturnType(removeRefsAndCVR(decl->getResultType()).getAsString());

		return result;

	}

	std::unique_ptr<Function> function(clang::FunctionDecl *decl)
	{
		return genericFunction<Function>(decl);
	}

	std::unique_ptr<Method> method(clang::CXXMethodDecl *decl)
	{
		std::unique_ptr<Method> result;

		for(auto attr : streams::stream(decl->specific_attr_begin<clang::AnnotateAttr>(),
		                                decl->specific_attr_end<clang::AnnotateAttr>()))
		{
			if(attr->getAnnotation().startswith("pygetter:"))
			{
				result = genericFunction<Getter>(decl, attr->getAnnotation().rsplit(':').second);
				break;
			}
			else if(attr->getAnnotation().startswith("pysetter:"))
			{
				result = genericFunction<Setter>(decl, attr->getAnnotation().rsplit(':').second);
				break;
			}
		}

		if(!result) result = genericFunction<Method>(decl);

		return result;
	}


};

} // namespace autobind

#endif // DATAEXTRACTOR_HPP_OPB0V8
