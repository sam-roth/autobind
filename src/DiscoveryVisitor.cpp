

#include "DiscoveryVisitor.hpp"
#include "util.hpp"
#include "transform.hpp"
#include "type.hpp"


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


auto attributeStream(clang::Decl &x)
{
	return streams::stream(x.specific_attr_begin<clang::AnnotateAttr>(),
	                       x.specific_attr_end<clang::AnnotateAttr>());
}

bool isPyExport(clang::Decl *d)
{
	using namespace streams;

	auto pred = [](auto a) { return a->getAnnotation() == "pyexport"; };
	return any(attributeStream(*d) | transformed(pred));
}


class DiscoveryVisitor
: public clang::RecursiveASTVisitor<DiscoveryVisitor>
{
	WrapperEmitter _wrapperEmitter;
	bool _foundModule = false;
	std::vector<clang::FunctionDecl *> _matches;
	autobind::ModuleManager &_modmgr;
	std::vector<autobind::Module *> _modstack;
public:

	explicit DiscoveryVisitor(clang::ASTContext *context, autobind::ModuleManager &modmgr)
	: _wrapperEmitter(context)
	, _modmgr(modmgr)
	{
				
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

	bool VisitCXXRecordDecl(clang::CXXRecordDecl *decl)
	{

		if(isPyExport(decl))
		{
			if(!checkInModule(decl)) return false;

			auto name = decl->getQualifiedNameAsString();
			auto unqualName = rsplit(name, "::").second;

// 			assert(!_modstack.empty());
			_modstack.back()->addExport(std::make_unique<Type>(unqualName,
			                                                   name,
			                                                   ""));

		}

		return true;
	}

	bool VisitNamespaceDecl(clang::NamespaceDecl *decl)
	{
		using namespace streams;

		auto stream = attributeStream(*decl)
			| filtered([](auto a) { return a->getAnnotation().startswith("py:module:"); });


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

// 
//          	if(auto comment = decl->getASTContext().getRawCommentForAnyRedecl(decl->getASTContext().getTranslationUnitDecl()))
// 			{
// 				if(//sm.getFileID(comment->getSourceRange().getBegin()) == fid && 
// 				   //std::abs(int(sm.getPresumedLineNumber(comment->getSourceRange().getEnd()) - curLine) < 3 &&
// 				   comment->isDocumentation())
// 				{
// 					_modstack.front()->setDocstring(comment->getRawText(decl->getASTContext().getSourceManager()));
// 				}
// 			}
		}

		auto docstringStream = attributeStream(*decl)
			| filtered([](auto a) { return a->getAnnotation().startswith("pydocstring:"); });

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

		if(isPyExport(decl))
		{
			_modstack.push_back(&_modmgr.module(decl->getNameAsString()));

			if(auto comment = decl->getASTContext().getRawCommentForAnyRedecl(decl))
			{
				if(comment->isDocumentation())
				{
					_modstack.back()->setDocstring(comment->getRawText(decl->getASTContext().getSourceManager()));
				}
			}
			
			pushed = true;
		}

		bool result = clang::RecursiveASTVisitor<DiscoveryVisitor>::TraverseNamespaceDecl(decl);


		if(pushed)
		{
			assert(!_modstack.empty());
			_modstack.pop_back();
		}

		return result;
	}

	bool VisitFunctionDecl(clang::FunctionDecl *decl)
	{
		
		if(isPyExport(decl))
		{
			if(!checkInModule(decl)) return false;
			_modstack.back()->addExport(_wrapperEmitter.function(decl));
		}
		

		return true;
	}

	const std::vector<clang::FunctionDecl *> &matches() const
	{
		return _matches;
	}
};

void discoverTranslationUnit(autobind::ModuleManager &mgr,
                             clang::TranslationUnitDecl &tu)
{
	DiscoveryVisitor dv(&tu.getASTContext(), mgr);
	dv.TraverseDecl(&tu);

}
} // autobind