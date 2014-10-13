// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include <string>
#include <fstream>

#include "util.hpp"

#include "DiscoveryVisitor.hpp"
#include "Export.hpp"


#include <clang/Frontend/FrontendPluginRegistry.h>

using namespace autobind;

class AutobindPluginAction: public clang::PluginASTAction
{
	std::string _outputFileName = "autobind_output.cpp";
protected:
	clang::ASTConsumer *CreateASTConsumer(clang::CompilerInstance &ci,
	                                      llvm::StringRef path)
	{
		std::string pathString = path;
		auto outputFileName = _outputFileName;
		return newASTConsumer([outputFileName, pathString, &ci](clang::ASTContext &ctx) {
			ModuleManager mgr;
			discoverTranslationUnit(mgr, *ctx.getTranslationUnitDecl(), ci);


			for(const auto &item : mgr.moduleStream())
			{
				mgr.module(item.first).setSourceTUPath(pathString);
			}

			std::ofstream outStream(outputFileName);
			mgr.codegen(outStream);
		});
	}

	bool ParseArgs(const clang::CompilerInstance &ci,
	               const std::vector<std::string> &args)
	{
		if(args.size() == 1)
		{
			_outputFileName = args[0];
		}

		return args.size() <= 1;
	}
};


static clang::FrontendPluginRegistry::Add<AutobindPluginAction> registration("autobind",
                                                                             "automatically generate "
                                                                             "Python bindings");





