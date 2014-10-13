// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef DIAGNOSTICSUPPRESSOR_HPP_BOCQTB
#define DIAGNOSTICSUPPRESSOR_HPP_BOCQTB

namespace autobind {

class DiagnosticSuppressor: public clang::DiagnosticConsumer
{
	std::unique_ptr<clang::DiagnosticConsumer> _next;
	std::unordered_set<unsigned> _ids;

	using Super = clang::DiagnosticConsumer;
public:
	DiagnosticSuppressor(std::unique_ptr<clang::DiagnosticConsumer> next,
	                     std::unordered_set<unsigned> suppressedIds);
	virtual ~DiagnosticSuppressor();

	void clear() override;
	void BeginSourceFile(const clang::LangOptions &langOpts,
	                     const clang::Preprocessor *pp=nullptr) override;
	void EndSourceFile() override;

	void HandleDiagnostic(clang::DiagnosticsEngine::Level diagLevel,
	                      const clang::Diagnostic &info) override;
};

} // autobind


#endif // DIAGNOSTICSUPPRESSOR_HPP_BOCQTB

