// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include <clang/AST/ASTContext.h>
#include "diagnostics.hpp"

namespace autobind {
namespace diag {

void emit(clang::DiagnosticsEngine::Level level, 
          const clang::Decl &decl,
          const std::string &msg)
{
	auto &diags = decl.getASTContext().getDiagnostics();
	unsigned id = diags.getCustomDiagID(level, msg);
	diags.Report(decl.getLocation(), id);
}


void stop(const clang::Decl &decl, const std::string &msg)
{
	emit(clang::DiagnosticsEngine::Error,
	     decl,
	     msg);

	throw DiagnosticError(decl, msg);
}

} // diag
} // autobind

