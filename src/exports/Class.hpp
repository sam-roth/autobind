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
	
	struct Accessor
	{
		const clang::CXXMethodDecl *getter=nullptr, *setter=nullptr;
		mutable std::string getterRef = "0", setterRef = "0";

		std::string escapedDocstring() const;
		void codegenDeclaration(const Class &parent, std::ostream &out) const;
		void codegen(const Class &parent, std::ostream &out) const;
	};

	Constructor _constructor;
	std::map<std::string, Accessor> _accessors;
	std::vector<std::unique_ptr<Func>> _functions;

public:
	Class(const clang::CXXRecordDecl &decl);

	const std::vector<std::unique_ptr<Func>> &functions() const { return _functions; }
	const std::string &selfTypeRef() const { return _selfTypeRef; }

	void setModuleName(const std::string &moduleName) { _moduleName = moduleName; }

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;
	virtual void codegenInit(std::ostream &) const override;
	virtual void merge(const Export &e) override;
};

} // autobind


#endif // CLASS_HPP_TDTVVJ

