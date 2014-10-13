// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include "DiagnosticSuppressor.hpp"

namespace autobind {


DiagnosticSuppressor::DiagnosticSuppressor(std::unique_ptr<clang::DiagnosticConsumer> next,
                                           std::unordered_set<unsigned int> suppressedIds)
{
	_next = std::move(next);
	_ids = std::move(suppressedIds);
}

DiagnosticSuppressor::~DiagnosticSuppressor()
{
}

void DiagnosticSuppressor::clear() 
{
	Super::clear();
	if(_next) _next->clear();
}


void DiagnosticSuppressor::BeginSourceFile(const clang::LangOptions &langOpts, const clang::Preprocessor *pp) 
{
	Super::BeginSourceFile(langOpts, pp);
	if(_next) _next->BeginSourceFile(langOpts, pp);
}


void DiagnosticSuppressor::EndSourceFile() 
{
	Super::EndSourceFile();
	if(_next) _next->EndSourceFile();
}


void DiagnosticSuppressor::HandleDiagnostic(clang::DiagnosticsEngine::Level lvl, const clang::Diagnostic &diag)
{
	if(_next && _ids.count(diag.getID()) == 0)
	{
		Super::HandleDiagnostic(lvl, diag);
		_next->HandleDiagnostic(lvl, diag);
	}
}

} // autobind


