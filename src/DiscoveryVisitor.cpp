// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include "DiscoveryVisitor.hpp"

#include "util.hpp"
#include "exports/Func.hpp"
#include "exports/Class.hpp"
#include "attributeStream.hpp"

#include <clang/Sema/Sema.h>
#include <clang/Sema/TemplateDeduction.h>
#include <unordered_set>
#include <array>

namespace autobind {

std::string prototypeSpelling(clang::FunctionDecl *decl)
{
	std::string result;
	bool first = true;

	result += decl->getReturnType().getAsString();
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

		unsigned id = diags.getCustomDiagID(clang::DiagnosticsEngine::Error, "%0");
		diags.Report(decl->getLocation(), id) << exc.what();
		return false;
	}
};

template <class T, size_t N>
llvm::ArrayRef<T> asArrayRef(const std::array<T, N> &arr)
{
	return llvm::makeArrayRef(arr.data(), arr.size());
}

class DiscoveryVisitor
: public clang::RecursiveASTVisitor<DiscoveryVisitor>
{
	bool _foundModule = false;
	std::vector<clang::FunctionDecl *> _matches;
	autobind::ModuleManager &_modmgr;
	std::vector<autobind::Module *> _modstack;
	int _namespaceDepth = 0;
	clang::CompilerInstance &_compiler;
	clang::ClassTemplateDecl *_pyConversion = 0;
	std::unordered_set<const clang::Type *> _knownTypes;


public:
	const clang::ASTContext &context;

	explicit DiscoveryVisitor(clang::ASTContext *context, 
	                          autobind::ModuleManager &modmgr,
	                          clang::CompilerInstance &compiler)
	: _modmgr(modmgr)
	, _compiler(compiler)
	, context(*context)
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

		std::array<clang::TemplateArgument, 2> args {{
			clang::TemplateArgument(clang::QualType(ty, 0).getCanonicalType()),
			clang::TemplateArgument(_pyConversion->getASTContext().VoidTy)
		}};

		void *insertPos = 0;


// 		clang::QualType pyConvQual(_pyConversion->type, 0);

// 		clang::QualifiedTemplateName tpl;
		auto tname = clang::TemplateName(_pyConversion);

// 		assert(!tname.isDependent());

		clang::TemplateArgumentList tal(clang::TemplateArgumentList::OnStack, args.data(), args.size());

// 		clang::TemplateArgumentListInfo talinfo;
// 		talinfo.addArgument(clang::TemplateArgumentLoc(args[0], clang::TemplateArgumentLocInfo{}));


		auto &sema = _compiler.getSema();



		llvm::SmallVector<clang::ClassTemplatePartialSpecializationDecl *, 12> parspecs;

		_pyConversion->getPartialSpecializations(parspecs);

		for(auto parspec : parspecs)
		{
// 			parspec->dumpColor();
			clang::sema::TemplateDeductionInfo info(clang::SourceLocation{});
			auto deduction = sema.DeduceTemplateArguments(parspec,
			                                              tal,
			                                              info);


			if(deduction == clang::Sema::TDK_Success)
			{
// 				std::cerr << "successful deduction: " << parspec->getNameAsString() << '\n';
// 				parspec->dumpColor();
				return true;
			}
		}

// 		for(auto spec = _pyConversion->spec_begin(); spec != _pyConversion->spec_end(); ++spec)
// 		{
// 			if(auto specPartial = llvm::dyn_cast_or_null<clang::ClassTemplatePartialSpecializationDecl>(*spec))
// 			{
// 
// 				specPartial->dumpColor();
// 			// 			sema.DeduceTemplateArguments(
// 			}
// 			else
// 			{
// 				spec->dumpColor();
// 			}
// //
// 		}
// 
// 		sema.DeduceTemplateArguments

// 		clang::Scope sc;
//
// 		auto scope = sema.getScopeForContext(_pyConversion->getDeclContext());
// 
// 
// 		llvm::SmallVector<clang::TemplateArgument, 4> talout;
// 
// 		sema.CheckTemplateArgumentList(_pyConversion,
// 		                               _pyConversion->getLocation(),
// 		                               talinfo,
// 		                               true,
// 		                               talout);
// 
// 
// 		for(const auto &item : talout)
// 		{
// 			item
// 		}
// 



// 		auto declres = sema.CheckClassTemplate(scope, clang::TTK_Class, clang::Sema::TUK_Reference,
// 		                                       clang::SourceLocation(), clang::CXXScopeSpec(),
// 		                                       _pyConversion->getIdentifier(), _pyConversion->getLocation(),
// 		                                       nullptr,
// 		                                       &

// 		talinfo.addArgument(



		// FIXME: I'm not sure what this is and what makes it 'canonical', but it seems to work.


//
// 		auto canonty = context.IntTy;
// 
// 
// 		auto specty = context.getTemplateSpecializationType(tname,
// 		                                                    args.data(),
// 		                                                    args.size(),
// 		                                                    canonty);
// 
// 		auto spectyCast = specty->getAs<clang::TemplateSpecializationType>();
// 		assert(spectyCast);
// 		spectyCast->dump();
// 		auto specCanonTy = spectyCast->getCanonicalTypeUnqualified();
// 		specCanonTy.dump();
// // 		assert(specdecl);
// 
// // 		specdecl->dumpColor();
// 
// 		specty.dump();


		return _pyConversion->findSpecialization(asArrayRef(args), insertPos);
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

	bool TraverseTranslationUnitDecl(clang::TranslationUnitDecl *decl)
	{
		bool result = clang::RecursiveASTVisitor<DiscoveryVisitor>::TraverseTranslationUnitDecl(decl);
		ConversionInfo info(*this);
		if(result)
		{
			for(const auto &module : _modmgr.moduleStream())
			{
				module.second.validate(info);
			}
		}
		return result;
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

		auto klass = ::autobind::makeUnique<Class>(*decl);
		klass->setModuleName(_modstack.back()->name());
		_modstack.back()->addExport(std::move(klass));
		_knownTypes.insert(decl->getTypeForDecl());

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
					auto func = ::autobind::makeUnique<Func>(decl->getNameAsString());
					func->addDecl(*funcDecl);
					_modstack.back()->addExport(std::move(func));
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
			auto func = ::autobind::makeUnique<Func>(decl->getNameAsString());
			func->addDecl(*decl);
			_modstack.back()->addExport(std::move(func));
        });
	}

	const std::vector<clang::FunctionDecl *> &matches() const
	{
		return _matches;
	}
};
bool ConversionInfo::ensureConversionSpecializationExists(const clang::Decl *decl, 
                                                          const clang::Type *tyPtr) const
{
// 	clang::QualType ty(tyPtr, 0);
	auto ty = tyPtr->getCanonicalTypeUnqualified();
	ty = ty.getNonReferenceType();

// 	ty.removeLocalVolatile();
// 	ty.removeLocalConst();
// 	ty.removeLocalRestrict();
// 	ty = ty.getCanonicalType();

	auto qty = clang::QualType(ty.getTypePtr(), 0);
	
	if(!willConversionSpecializationExist(ty.getTypePtr()))
	{
		auto &diags = _parent.context.getDiagnostics();
		unsigned id = diags.getCustomDiagID(clang::DiagnosticsEngine::Error, 
		                                    "No specialization of python::Conversion for type %0");
		diags.Report(decl->getLocation(), id) << qty;
		return false;
	}


	return true;
}

bool ConversionInfo::willConversionSpecializationExist(const clang::Type *ty) const
{
	return _parent.willConversionSpecializationExist(ty);
}

void discoverTranslationUnit(autobind::ModuleManager &mgr,
                             clang::TranslationUnitDecl &tu,
                             clang::CompilerInstance &compiler)
{
	DiscoveryVisitor dv(&tu.getASTContext(), mgr, compiler);
	dv.TraverseDecl(&tu);

}
} // autobind
