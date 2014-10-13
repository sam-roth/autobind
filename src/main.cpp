// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

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

}}  // llvm::yaml


namespace autobind {


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

	const char *emitYamlDiag = getenv("AB_EMIT_YAML_DIAG");

	llvm::cl::OptionCategory category("Autobind Options");
	CommonOptionsParser optparse(argc, argv, category);


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

