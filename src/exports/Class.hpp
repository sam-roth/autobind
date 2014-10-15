// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef CLASS_HPP_TDTVVJ
#define CLASS_HPP_TDTVVJ

#include <vector>
#include <map>
#include <memory>
#include "../Export.hpp"
#include "Func.hpp"
#include "../ClassData.hpp"

namespace clang 
{ 
	class CXXConstructorDecl;
	class CXXMethodDecl;
}

namespace autobind {

class Func;

class Class: public Export
{
	const clang::CXXRecordDecl &_decl;
	std::string _selfTypeRef;
	std::string _moduleName;

	ClassData _classData;

	Constructor _constructor;
	std::map<std::string, std::unique_ptr<ClassExport>> _exports;
	void mergeClassExport(std::unique_ptr<ClassExport>);
public:
	Class(const clang::CXXRecordDecl &decl);

	const ClassData &classData() const { return _classData; }
	const std::string &selfTypeRef() const { return _selfTypeRef; }

	void setModuleName(const std::string &moduleName) { _moduleName = moduleName; }

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;
	virtual void codegenInit(std::ostream &) const override;
	virtual void merge(const Export &e) override;

	bool validate(const autobind::ConversionInfo &) const override;
};

} // autobind


#endif // CLASS_HPP_TDTVVJ

