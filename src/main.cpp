// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include <iostream>
#include <utility>
#include <vector>
#include <unordered_map>

#include <boost/regex.hpp>

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

#include "util.hpp"
#include "diagnostics.hpp"
#include "Export.hpp"

namespace autobind {



static llvm::cl::OptionCategory toolCat("autobind options");


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
		return newASTConsumer([inFile, &compiler](clang::ASTContext &ctx) {
			ModuleManager mgr;
			discoverTranslationUnit(mgr, *ctx.getTranslationUnitDecl(), compiler);
			
			
			for(const auto &item : mgr.moduleStream())
			{
				mgr.module(item.first).setSourceTUPath(inFile);
			}

			try
			{
				mgr.codegen(std::cout);
			}
			catch(DiagnosticError &e)
			{
				// already presented error, so eat the exception
			}
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

