// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include <clang/AST/DeclCXX.h>

#include "ClassData.hpp"
#include "util.hpp"

namespace autobind {



ClassData::ClassData(const clang::CXXRecordDecl &decl)
: _decl(decl)
, _wrapperRef(gensym(decl.getNameAsString()))
, _typeRef(decl.getQualifiedNameAsString())
{

}

std::string ClassData::exportName() const
{
	return _decl.getNameAsString();
}

bool ClassData::isDefaultConstructible() const
{
	return _decl.hasDefaultConstructor();
}



} // autobind




