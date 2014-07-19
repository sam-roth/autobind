// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef CLASSDATA_HPP_MS8SMK
#define CLASSDATA_HPP_MS8SMK

#include <string>

namespace clang { class CXXRecordDecl; }

namespace autobind {

class ClassData
{
	const clang::CXXRecordDecl &_decl;
	const std::string _wrapperRef;
	const std::string _typeRef;
public:
	ClassData(const clang::CXXRecordDecl &decl);

	const clang::CXXRecordDecl &decl() const { return _decl; }
	const std::string &wrapperRef() const { return _wrapperRef; }
	const std::string &typeRef() const { return _typeRef; }
	std::string exportName() const;

	bool isDefaultConstructible() const;
};

} // autobind

#endif // CLASSDATA_HPP_MS8SMK

