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
#include "llvm/Support/YAMLTraits.h"
#include "clang/Frontend/ChainedDiagnosticConsumer.h"

#include "DiscoveryVisitor.hpp"

#include "util.hpp"
#include "diagnostics.hpp"
#include "Export.hpp"

namespace autobind {

struct DiagnosticRecord
{
	std::string filename;
	int line;
	int col;
	std::string message;

	DiagnosticRecord() { }
	DiagnosticRecord(const clang::Diagnostic &diag)
	{
		assert(diag.hasSourceManager());

		auto &sm = diag.getSourceManager();
		clang::FullSourceLoc loc(diag.getLocation(), sm);
		filename = sm.getFilename(diag.getLocation());
		line = loc.getSpellingLineNumber();
		col = loc.getSpellingColumnNumber();
		llvm::SmallVector<char, 128> msgVec;
		diag.FormatDiagnostic(msgVec);
		message.assign(msgVec.begin(), msgVec.end());

	}
};

}

namespace llvm { namespace yaml {

template <>
struct MappingTraits<autobind::DiagnosticRecord>
{
	static void mapping(IO &io, autobind::DiagnosticRecord &dr)
	{
		io.mapRequired("filename", dr.filename);
		io.mapRequired("line", dr.line);
		io.mapRequired("col", dr.col);
		io.mapRequired("message", dr.message);
	}
};
// template <> 
// struct ScalarTraits<std::string>
// {
// 	// backported from newer llvm
// 
// 	static void output(const std::string &value, void *, llvm::raw_ostream &out)
// 	{
// 		out << value;
// 	}
// 
// 	static StringRef input(StringRef scalar, void *, std::string &val) {
// 		val = scalar;
// 		return StringRef();
// 	}
// 
// 	static bool mustQuote(StringRef) { return true; }
// };

}}  // llvm::yaml


namespace autobind {


static llvm::cl::OptionCategory toolCat("autobind options");


namespace severity
{
	const auto Error     = clang::DiagnosticsEngine::Error;
	const auto Warning   = clang::DiagnosticsEngine::Warning;
	const auto Note      = clang::DiagnosticsEngine::Note;
};

// void emitMsg(clang::SourceManager &mgr,
//              clang::DiagnosticsEngine &diags,
//              clang::SourceLocation loc,
//              const std::string &message,
//              clang::DiagnosticsEngine::Level severity)
// {
// 
// 	clang::FullSourceLoc fullLoc(loc, mgr);
// 	unsigned id = diags.getCustomDiagID(severity, message);
// 	clang::DiagnosticBuilder b = diags.Report(fullLoc, id);
// 	b.setForceEmit();
// }
// 
// class PythonPragmaHandler: public clang::PragmaHandler
// {
// public:
// 	virtual void HandlePragma(clang::Preprocessor &pp,
// 	                          clang::PragmaIntroducerKind introducer,
// 	                          clang::Token &firstToken) override
// 	{
//
// 		if(!clang::tok::isStringLiteral(firstToken.getKind()))
// 		{
// 
// 			emitMsg(pp.getSourceManager(),
// 			        pp.getDiagnostics(),
// 			        firstToken.getLocation(),
// 			        "`#pragma pymodule` must be followed by a string literal",
// 			        severity::Error);
// 			return;
// 		}
// 		else
// 		{
// 
// 			auto string = clang::StringLiteralParser(&firstToken, 1, pp).GetString();
// 
// 			emitMsg(pp.getSourceManager(),
// 			        pp.getDiagnostics(),
// 			        firstToken.getLocation(),
// 			        (boost::format("Found token with literal data: '%1%'.") % std::string(string)).str(),
// 			        severity::Note);
// 		}
// 	}
// };
// 
class FindFunctionDeclsAction: public clang::ASTFrontendAction
{

public:
	/// If non-null when BeginInvocation() is called, this diagnostic consumer will be chained
	/// with the existing diagnostic consumer. Ownership is transferred to the compiler instance.

	clang::DiagnosticConsumer *diagnosticConsumer = nullptr;

	virtual bool BeginInvocation(clang::CompilerInstance &ci) override
	{
		// set the diagnostic consumer, if necessary
		if(diagnosticConsumer)
		{
			assert(ci.hasDiagnostics());

			ci.getDiagnostics().setClient(new clang::ChainedDiagnosticConsumer(diagnosticConsumer,
			                                                                   ci.getDiagnostics().takeClient()));
		}

		return true;
	}

	virtual bool BeginSourceFileAction(clang::CompilerInstance &ci, llvm::StringRef filename) override
	{
		bool result = clang::ASTFrontendAction::BeginSourceFileAction(ci, filename);
		clang::Preprocessor &preproc = ci.getPreprocessor();
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

/// Serializes diagnostics to YAML format.
class SerializingDiagnosticConsumer: public clang::DiagnosticConsumer
{
	llvm::yaml::Output out;

public:
	SerializingDiagnosticConsumer(llvm::raw_ostream &os)
	: out(os)
	{ }

protected:
	virtual void HandleDiagnostic(clang::DiagnosticsEngine::Level diagLevel,
	                              const clang::Diagnostic &info) override
	{
		DiagnosticRecord dr(info);
		out << dr;
	}
};


template <class F>
clang::tooling::FrontendActionFactory *makeFrontendActionFactory(F func)
{
	class Result: public clang::tooling::FrontendActionFactory
	{
		F func;
	public:
		Result(F func): func(std::move(func)) { }
		clang::FrontendAction *create() override
		{
			return func();
		}
	};


	return new Result(std::move(func));
}

}


int main(int argc, const char **argv)
{

	using namespace autobind;
	using namespace clang::tooling;

	for(const char **arg = argv; *arg; ++arg)
	{
		std::cerr << *arg << "\n";
	}

	const char *emitYamlDiag = getenv("AB_EMIT_YAML_DIAG");

	llvm::cl::OptionCategory category("Autobind Options");
	CommonOptionsParser optparse(argc, argv, category);


// 	return 0;

	ClangTool tool(optparse.getCompilations(),
	               optparse.getSourcePathList());
	
	std::string errorInfo;
	std::unique_ptr<llvm::raw_fd_ostream> fout;
	if(emitYamlDiag)
	{
		fout.reset(new llvm::raw_fd_ostream(emitYamlDiag, errorInfo, llvm::sys::fs::F_Text));
		if(fout->has_error())
		{
			std::cerr << "Warning: " << errorInfo << "\n";
		}
	}

	int result = tool.run(makeFrontendActionFactory([&]{
		auto result = new FindFunctionDeclsAction;
		if(fout && !fout->has_error())
		{
			result->diagnosticConsumer = new SerializingDiagnosticConsumer(*fout);
		}
		return result;
	}));

	return result;
}

