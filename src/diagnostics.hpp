// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef DIAGNOSTICS_HPP_JOM2LC
#define DIAGNOSTICS_HPP_JOM2LC

#include <clang/Basic/Diagnostic.h>
#include <clang/AST/Decl.h>

#include <stdexcept>

namespace autobind {

/// Thrown after reporting an error to the user.
///
/// Use DiagnosticError when stack unwinding semantics are appropriate
/// after a diagnostic has been emitted.
struct DiagnosticError: public std::runtime_error
{
	const clang::Decl &decl;

	DiagnosticError(const clang::Decl &decl, 
	                const std::string &msg)
	: std::runtime_error(msg)
	, decl(decl)
	{ }
};

namespace diag {

void emit(clang::DiagnosticsEngine::Level level, 
          const clang::Decl &d,
          const std::string &msg);


/// Emit an error diagnostic and throw DiagnosticError.
void stop(const clang::Decl &decl,
          const std::string &msg);

} // diag
} // autobind


#endif // DIAGNOSTICS_HPP_JOM2LC






