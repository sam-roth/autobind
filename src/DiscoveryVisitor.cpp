

#include "DiscoveryVisitor.hpp"
#include "util.hpp"
#include "transform.hpp"

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
	std::vector<clang::FunctionDecl *> _matches;
	autobind::ModuleManager &_modmgr;
	std::vector<autobind::Module *> _modstack;
public:

	explicit DiscoveryVisitor(clang::ASTContext *context, autobind::ModuleManager &modmgr)
	: _wrapperEmitter(context)
	, _modmgr(modmgr)
	{
		
	}

	bool VisitNamespaceDecl(clang::NamespaceDecl *decl)
	{
		using namespace streams;

		auto stream = attributeStream(*decl)
			| filtered([](auto a) { return a->getAnnotation().startswith("py:module:"); });


		if(!stream.empty())
		{
			auto parts = stream.front()->getAnnotation().rsplit(':');
			_modstack.insert(_modstack.begin(), &_modmgr.module(parts.second));

			stream.next();
			assert(stream.empty() && "multiple module names declared");

		}


		return true;
	}


	bool TraverseNamespaceDecl(clang::NamespaceDecl *decl)
	{

		bool pushed = false;

		if(isPyExport(decl))
		{
			_modstack.push_back(&_modmgr.module(decl->getNameAsString()));
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

			assert(!_modstack.empty() && "python exports must be preceded by module declaration");
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