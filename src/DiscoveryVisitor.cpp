// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include "DiscoveryVisitor.hpp"
#include "DataExtractor.hpp"

#include "util.hpp"
#include "Type.hpp"
#include "exports/Func.hpp"
#include "exports/Class.hpp"
#include "attributeStream.hpp"

#include <clang/Sema/Sema.h>
#include <unordered_set>

namespace autobind {

std::string prototypeSpelling(clang::FunctionDecl *decl)
{
	std::string result;
	bool first = true;

	result += decl->getResultType().getAsString();
	result += " ";
	result += decl->getQualifiedNameAsString();

	result += "(";

	for(auto param : PROP_RANGE(decl->param))
	{
		if(first) first = false;
		else result += ", ";

		result += param->getType().getAsString();
		result += " ";
		result += param->getNameAsString();

	}
	result += ")";

	return result;
}

template <class Decl, class Func>
bool errorToDiag(Decl decl, const Func &func) {
	try
	{
		func();
		return true;
	}
	catch(std::exception &exc)
	{
		auto &diags = decl->getASTContext().getDiagnostics();
		unsigned id = diags.getCustomDiagID(clang::DiagnosticsEngine::Error, exc.what());
		diags.Report(decl->getLocation(), id);
		return false;
	}
};


class DiscoveryVisitor
: public clang::RecursiveASTVisitor<DiscoveryVisitor>
{
	DataExtractor _wrapperEmitter;
	bool _foundModule = false;
	std::vector<clang::FunctionDecl *> _matches;
	autobind::ModuleManager &_modmgr;
	std::vector<autobind::Module *> _modstack;
	int _namespaceDepth = 0;
	clang::CompilerInstance &_compiler;
	clang::ClassTemplateDecl *_pyConversion = 0;
	std::unordered_set<const clang::Type *> _knownTypes;

public:

	explicit DiscoveryVisitor(clang::ASTContext *context, 
	                          autobind::ModuleManager &modmgr,
	                          clang::CompilerInstance &compiler)
	: _wrapperEmitter(context)
	, _modmgr(modmgr)
	, _compiler(compiler)
	{
		assert(_compiler.hasSema());

		_knownTypes = {
			context->IntTy.getTypePtr(),
			context->getPointerType(context->getConstType(context->CharTy)).getTypePtr()
		};
	}

	bool willConversionSpecializationExist(const clang::Type *ty)
	{
		if(!_pyConversion)
		{
			throw std::runtime_error("Must include autobind.hpp before declaring exports.");
		}

		if(_knownTypes.count(ty)) return true; // we will emit this specialization ourselves

		clang::TemplateArgument args[] = {
			clang::TemplateArgument(clang::QualType(ty, 0)),
			clang::TemplateArgument(_pyConversion->getASTContext().VoidTy)
		};

		void *insertPos = 0;

		return _pyConversion->findSpecialization(args, 2, insertPos) != nullptr;
	}


	void validateExportedFunctionDecl(const clang::FunctionDecl *func)
	{
		// TODO: defer this until after visiting the entire TU

#if 0
		using namespace streams;
		for(auto param : stream(func->param_begin(),
		                        func->param_end()))
		{
			auto ty = param->getType().getNonReferenceType();
			ty.removeLocalVolatile();
			ty.removeLocalConst();
			ty.removeLocalRestrict();
			ty = ty.getCanonicalType();

			if(!willConversionSpecializationExist(ty.getTypePtr()))
			{
				_pyConversion->dumpColor();
				for(auto spec : stream(_pyConversion->spec_begin(),
				                       _pyConversion->spec_end()))
				{
					spec->dumpColor();
				}
				throw std::runtime_error("No specialization of python::Conversion for type " + ty.getAsString());
			}
		}
#endif
	}

	bool VisitClassTemplateDecl(clang::ClassTemplateDecl *decl)
	{
		if(decl->getQualifiedNameAsString() == "python::Conversion")
		{
			_pyConversion = decl;
		}
		return true;
	}


	bool checkInModule(clang::Decl *d)
	{
		if(_modstack.empty())
		{
			auto &diags = d->getASTContext().getDiagnostics();
			unsigned id = diags.getCustomDiagID(clang::DiagnosticsEngine::Error,
			                                    "python exports must be preceded by module declaration");
			diags.Report(d->getLocation(), id);
			return false;
		}

		return true;
	}

	void handleExportRecordDecl(clang::CXXRecordDecl *decl)
	{
// 		using namespace streams;

		auto klass = ::autobind::make_unique<Class>(*decl);
		klass->setModuleName(_modstack.back()->name());
		_modstack.back()->addExport(std::move(klass));

	}


	bool VisitUsingDecl(clang::UsingDecl *decl)
	{
		// check if we're in a pyexport namespace, rather than at module scope
		if(_namespaceDepth == 0) return true;
		// shouldn't happen, but check anyway
		if(!checkInModule(decl)) return false;

		return errorToDiag(decl, [&]{
			using namespace streams;
			for(auto shadow : stream(decl->shadow_begin(),
			                         decl->shadow_end()))
			{
				auto target = shadow->getTargetDecl();
				if(auto recordDecl = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(target))
				{
					this->handleExportRecordDecl(recordDecl);
				}
				else if(auto funcDecl = llvm::dyn_cast_or_null<clang::FunctionDecl>(target))
				{
					_modstack.back()->addExport(_wrapperEmitter.function(funcDecl));
				}
			}
        });
		return true;
	}

	bool VisitCXXRecordDecl(clang::CXXRecordDecl *decl)
	{
		using namespace streams;

		if(!isPyExport(decl)) return true;
		if(!checkInModule(decl)) return false;

		return errorToDiag(decl, [&]{
			this->handleExportRecordDecl(decl);
        });
	}

	bool VisitNamespaceDecl(clang::NamespaceDecl *decl)
	{
		using namespace streams;

		auto stream = attributeStream(*decl)
			| filtered([](const clang::AnnotateAttr *a) { return a->getAnnotation().startswith("py:module:"); });


		if(!stream.empty())
		{
			if(_foundModule)
			{
				auto &diags = decl->getASTContext().getDiagnostics();
				unsigned id = diags.getCustomDiagID(clang::DiagnosticsEngine::Error,
				                                    "redeclaration of module name");

				diags.Report(decl->getLocation(), id);

				return false;
			}

			auto parts = stream.front()->getAnnotation().rsplit(':');
			_modstack.insert(_modstack.begin(), &_modmgr.module(parts.second));
			_foundModule = true;

		}

		auto docstringStream = attributeStream(*decl)
			| filtered([](const clang::AnnotateAttr *a) { return a->getAnnotation().startswith("pydocstring:"); });
			
		if(!docstringStream.empty())
		{
			auto ann = docstringStream.front()->getAnnotation();
			checkInModule(decl);

			_modstack.back()->setDocstring(ann.split(':').second);
		}


		return true;
	}


	bool TraverseNamespaceDecl(clang::NamespaceDecl *decl)
	{

		bool pushed = false;
		bool added = false;

		if(isPyExport(decl))
		{
			added = true;
			++_namespaceDepth;
			if(!decl->isAnonymousNamespace())
			{
				_modstack.push_back(&_modmgr.module(decl->getNameAsString()));

				if(auto comment = decl->getASTContext().getRawCommentForAnyRedecl(decl))
				{
					if(comment->isDocumentation())
					{
						_modstack.back()->setDocstring(comment->getRawText(decl->getASTContext().getSourceManager()));
					}
				}

				++_namespaceDepth;

				pushed = true;
			}
		}

		bool result = clang::RecursiveASTVisitor<DiscoveryVisitor>::TraverseNamespaceDecl(decl);


		if(added)
		{
			--_namespaceDepth;
		}

		if(pushed)
		{
			assert(!_modstack.empty());
			_modstack.pop_back();
		}

		return result;
	}


	bool VisitFunctionDecl(clang::FunctionDecl *decl)
	{
		if(!isPyExport(decl)) return true;
		if(!checkInModule(decl)) return false;

		return errorToDiag(decl, [&]{
			this->validateExportedFunctionDecl(decl);
			auto func = ::autobind::make_unique<Func>(decl->getNameAsString());
			func->addDecl(*decl);
			_modstack.back()->addExport(std::move(func));
// 			_modstack.back()->addExport(_wrapperEmitter.function(decl));
        });
	}

	const std::vector<clang::FunctionDecl *> &matches() const
	{
		return _matches;
	}
};

void discoverTranslationUnit(autobind::ModuleManager &mgr,
                             clang::TranslationUnitDecl &tu,
                             clang::CompilerInstance &compiler)
{
	DiscoveryVisitor dv(&tu.getASTContext(), mgr, compiler);
	dv.TraverseDecl(&tu);

}
} // autobind
