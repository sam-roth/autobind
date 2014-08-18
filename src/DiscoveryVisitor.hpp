// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef DISCOVERYVISITOR_HPP_25YR9D
#define DISCOVERYVISITOR_HPP_25YR9D

#include "prefix.hpp"
#include "Export.hpp"
#include "ModuleManager.hpp"

namespace autobind {


void discoverTranslationUnit(autobind::ModuleManager &mgr,
                             clang::TranslationUnitDecl &tu,
                             clang::CompilerInstance &compiler);

class DiscoveryVisitor;
class ConversionInfo
{
	DiscoveryVisitor &_parent;
public:
	ConversionInfo(DiscoveryVisitor &parent)
	: _parent(parent) { }

	bool willConversionSpecializationExist(const clang::Type *ty) const;
	bool ensureConversionSpecializationExists(const clang::Decl *decl, const clang::Type *ty) const;
};


} // autobind


#endif // DISCOVERYVISITOR_HPP_25YR9D

