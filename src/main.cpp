
#include <iostream>
#include <utility>
#include <vector>
#include <unordered_map>

#include "clang/AST/AttrIterator.h"
#include "clang/AST/Attr.h"
#include "clang/Frontend/CompilerInstance.h"

#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Pragma.h"
#include "clang/Lex/LiteralSupport.h"

#include "DiscoveryVisitor.hpp"

// #include "symident.hpp"
#include "transform.hpp"
#include "util.hpp"
#include "data.hpp"

namespace autobind {

std::unordered_map<std::string, size_t> gensyms;

std::string gensym(const std::string &prefix)
{
	auto it = gensyms.find(prefix);
	if(it == gensyms.end())
	{
		gensyms[prefix] = 1;
		return prefix + "$0";
	}
	else
	{
		size_t oldCount = it->second;
		++it->second;
		return prefix + "$" + std::to_string(oldCount);
	}
}

static llvm::cl::OptionCategory toolCat("hgr options");
// 
// class FindFunctionDeclsConsumer: public clang::ASTConsumer
// {
// 	autobind::ModuleManager _modmgr;
// public:
// 
// 	explicit FindFunctionDeclsConsumer(clang::ASTContext *context)
// 	{
// 	}
// 
// 	virtual void HandleTranslationUnit(clang::ASTContext &context)
// 	{
// 		discoverTranslationUnit(_modmgr, *context.getTranslationUnitDecl());
// 		_modmgr.codegen(std::cout);
// 	}
// };
// 

template <class F>
clang::ASTConsumer *newASTConsumer(F func)
{
	struct ResultType: public clang::ASTConsumer
	{
		F _func;
		ResultType(F func): _func(std::move(func)) { }

		void HandleTranslationUnit(clang::ASTContext &context) final override
		{
			_func(context);
		}
	};

	return new ResultType(std::move(func));
}


namespace severity
{
	const auto Error     = clang::DiagnosticsEngine::Error;
	const auto Warning   = clang::DiagnosticsEngine::Warning;
	const auto Note      = clang::DiagnosticsEngine::Note;
};

void emitMsg(clang::SourceManager &mgr,
             clang::DiagnosticsEngine &diags,
             clang::SourceLocation loc,
             const std::string &message,
             clang::DiagnosticsEngine::Level severity)
{

	clang::FullSourceLoc fullLoc(loc, mgr);
	unsigned id = diags.getCustomDiagID(severity, message);
	clang::DiagnosticBuilder b = diags.Report(fullLoc, id);
	b.setForceEmit();
}

class PythonPragmaHandler: public clang::PragmaHandler
{
public:
	virtual void HandlePragma(clang::Preprocessor &pp,
	                          clang::PragmaIntroducerKind introducer,
	                          clang::Token &firstToken) override
	{

		if(!clang::tok::isStringLiteral(firstToken.getKind()))
		{
			emitMsg(pp.getSourceManager(),
			        pp.getDiagnostics(),
			        firstToken.getLocation(),
			        "`#pragma pymodule` must be followed by a string literal",
			        severity::Error);
			return;
		}
		else
		{

			auto string = clang::StringLiteralParser(&firstToken, 1, pp).GetString();

			emitMsg(pp.getSourceManager(),
			        pp.getDiagnostics(),
			        firstToken.getLocation(),
			        (boost::format("Found token with literal data: '%1%'.") % std::string(string)).str(),
			        severity::Note);
		}
	}
};

class FindFunctionDeclsAction: public clang::ASTFrontendAction
{

public:
	virtual bool BeginSourceFileAction(clang::CompilerInstance &ci, llvm::StringRef filename) override
	{
		bool result = clang::ASTFrontendAction::BeginSourceFileAction(ci, filename);
		clang::Preprocessor &preproc = ci.getPreprocessor();
		preproc.AddPragmaHandler("pymodule", new PythonPragmaHandler);
		return result;
	}

	virtual clang::ASTConsumer
	*CreateASTConsumer(clang::CompilerInstance &compiler,
	                   llvm::StringRef inFile) override
	{
		return newASTConsumer([inFile](clang::ASTContext &ctx) {
			ModuleManager mgr;
			discoverTranslationUnit(mgr, *ctx.getTranslationUnitDecl());

			
			for(const auto &item : mgr.moduleStream())
			{
				mgr.module(item.first).setSourceTUPath(inFile);
			}

			mgr.codegen(std::cout);
		});
	}
};

}


int main(int argc, const char **argv)
{

	using namespace autobind;
	using namespace clang::tooling;

	CommonOptionsParser optparse(argc, argv);



	ClangTool tool(optparse.getCompilations(),
	               optparse.getSourcePathList());




	return tool.run(newFrontendActionFactory<FindFunctionDeclsAction>());


}

