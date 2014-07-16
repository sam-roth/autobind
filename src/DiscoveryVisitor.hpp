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



} // autobind


#endif // DISCOVERYVISITOR_HPP_25YR9D

