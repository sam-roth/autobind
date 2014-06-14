#pragma once

#include <algorithm>
#include <string>
#include <iostream>
#include <map>
#include <typeinfo>

#include "clang/AST/AttrIterator.h"
#include "clang/AST/Attr.h"
#include "clang/Frontend/CompilerInstance.h"

#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include "util.hpp"



class SymIdentLocatingVisitor
: public clang::RecursiveASTVisitor<SymIdentLocatingVisitor>
{
	std::map<std::string, clang::QualType> _entries;
public:
	bool VisitCXXRecordDecl(clang::CXXRecordDecl *decl)
	{
		auto annotIsPySymident = [](clang::AnnotateAttr *a) { return a->getAnnotation() == "pysymident"; };

		if(std::any_of(decl->specific_attr_begin<clang::AnnotateAttr>(),
		               decl->specific_attr_end<clang::AnnotateAttr>(),
		               annotIsPySymident))
		{
			// found the symbol identities

			for(auto member : PROP_RANGE(decl->decls))
			{
				if(auto field = llvm::dyn_cast_or_null<clang::TypedefDecl>(member))
				{
					std::cout << "---> found symident entry " << 
						field->getNameAsString() << " of type " <<
						field->getUnderlyingType().getAsString() << "\n";

					_entries[field->getUnderlyingType().getAsString()] =
						field->getUnderlyingType();


					auto underlyingTy = field->getUnderlyingType();

// 					underlyingTy.getType

					std::cout << " type class: " << underlyingTy.getTypePtr()
						->getCanonicalTypeUnqualified().getTypePtr()->getTypeClassName()
						<< "\n";

					auto tst = underlyingTy.getTypePtr()->getAs<clang::TemplateSpecializationType>();
					if(tst)
					{
						std::cout << "tpl: " << tst->getTemplateName().getAsTemplateDecl()->getNameAsString()
						<< "\n";
					}
// 					std::cout << " type of type object: " << typeid(*underlyingTy.getTypePtr()).name() << "\n";
				}
			}

			return false; // found it -- stop
		}

		return true; // keep going
	}

	const std::map<std::string, clang::QualType> &
	entries() const
	{
		return _entries;
	}


	std::map<std::string, clang::TemplateName>
	templateNames() const
	{
		std::map<std::string, clang::TemplateName> result;


		const auto &dummyTy = _entries.at("DummyType");

		for(auto item : _entries)
		{
			if(auto templateTy = item.second.getTypePtr()->getAs<clang::TemplateSpecializationType>())
			{
				if(templateTy->getNumArgs() >= 1 && templateTy->getArg(0).getAsType() == dummyTy)
				{
					result[item.first] = templateTy->getTemplateName();
				}
			}
		}


		return result;
	}
};




