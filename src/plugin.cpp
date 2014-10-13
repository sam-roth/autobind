
#include <string>
#include <fstream>

#include "util.hpp"

#include "DiscoveryVisitor.hpp"
#include "Export.hpp"

#include <clang/Frontend/FrontendPluginRegistry.h>

using namespace autobind;

class AutobindPluginAction: public clang::PluginASTAction
{
protected:
	clang::ASTConsumer *CreateASTConsumer(clang::CompilerInstance &ci,
	                                      llvm::StringRef path)
	{
		std::string pathString = path;
		return newASTConsumer([pathString, &ci](clang::ASTContext &ctx) {
			ModuleManager mgr;
			discoverTranslationUnit(mgr, *ctx.getTranslationUnitDecl(), ci);


			for(const auto &item : mgr.moduleStream())
			{
				mgr.module(item.first).setSourceTUPath(pathString);
			}
			
			std::ofstream outStream("/tmp/output.cpp");
			mgr.codegen(outStream);
		});
	}

	bool ParseArgs(const clang::CompilerInstance &ci,
	               const std::vector<std::string> &args)
	{
		return true;
	}


};


static clang::FrontendPluginRegistry::Add<AutobindPluginAction> registration("autobind",
                                                                             "automatically generate "
                                                                             "Python bindings");





