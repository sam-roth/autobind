#include <iostream>
#include <clang/AST/ASTContext.h>
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/Attr.h"

#include "util.hpp"
#include "StringTemplate.hpp"
#include "TupleUnpacker.hpp"
#include "CallGenerator.hpp"
#include "exports/Func.hpp"
#include "exports/Class.hpp"

#include "Module.hpp"

#include "regex.hpp"
#include "stream.hpp"
#include "printing.hpp"
static void testStringTemplate()
{
	autobind::StringTemplate stmp = R"EOF(
	b
	a{{b}}
	a
		v

			c
	)EOF";
	


	stmp.into(std::cout)
		.set("b", "")
		.expand();

}

static void testRegexReplace()
{
	const char *text = R"EOF(
		It is the programmer's responsibility to ensure that the std::basic_regex
		object passed to the iterator's constructor outlives the iterator. Because
		the iterator stores a pointer to the regex, incrementing the iterator after
		the regex was destroyed accesses a dangling pointer. If the part of the
		regular expression that matched is just an assertion (^, $, \b, \B), the
		match stored in the iterator is a zero-length match, that is,
		match[0].first == match[0].second.
	)EOF";
	const char *expected = R"EOF(
		It is the pemmargorr's rtilibisnopsey to erusne taht the std::beger_cisax
		ocejbt pessad to the iotaretr's cotcurtsnor oeviltus the iotaretr. Bsuacee
		the iotaretr serots a petnior to the regex, initnemercng the iotaretr aetfr
		the regex was deyortsed aesseccs a dnilgnag petnior. If the prat of the
		raluger eoisserpxn taht mehctad is jsut an aoitressn (^, $, \b, \B), the
		mctah serotd in the iotaretr is a zreo-ltgneh mctah, taht is,
		mctah[0].fsrit == mctah[0].snoced.
	)EOF";

	auto repl = [&](const autobind::regex::smatch &m) {
		std::string mid = m[2];
		std::reverse(mid.begin(), mid.end());

		return m[1].str() + mid + m[3].str();
	};

	autobind::regex::regex pattern("(\\w)(\\w+)(\\w)");

	auto munged = autobind::regex_replace(std::string(text), pattern, repl);
	assert(munged == expected);
	// for good measure
	auto reconstructed = autobind::regex_replace(munged, pattern, repl);
	assert(reconstructed == text);
}

template <class F>
clang::ASTFrontendAction *newASTFrontendAction(F &&func)
{
	class Result: public clang::ASTFrontendAction
	{
		F &&func;
	public:
		Result(F &&func)
		: func(func) { }

		virtual clang::ASTConsumer *CreateASTConsumer(
	    clang::CompilerInstance &compiler, llvm::StringRef inFile) override
		{
			return newASTConsumer(func);
		}
	};

	return new Result(func);
}

bool hasAnnotation(const clang::Decl &decl, llvm::StringRef ann)
{
	using namespace streams;
	for(auto a : stream(decl.specific_attr_begin<clang::AnnotateAttr>(),
	                    decl.specific_attr_end<clang::AnnotateAttr>()))
	{
		if(a->getAnnotation() == ann) return true;
	}

	return false;
}

int main()
{
	testStringTemplate();
	testRegexReplace();

	auto testTupleUnpacker = [&](clang::ASTContext &ctx) {
		auto tu = ctx.getTranslationUnitDecl();
		autobind::Module mod;
		mod.setName("testmod");
		for(auto decl : streams::stream(tu->decls_begin(), tu->decls_end()))
		{
			if(!hasAnnotation(*decl, "pyexport")) continue;

			if(auto funcDecl = llvm::dyn_cast_or_null<clang::FunctionDecl>(decl))
			{
				funcDecl->dumpColor();

				auto f = autobind::makeUnique<autobind::Func>(funcDecl->getQualifiedNameAsString());
				f->addDecl(*funcDecl);
				mod.addExport(std::move(f));
			}
		}

		mod.setSourceTUPath("?");
		mod.codegen(std::cout);
	};


	clang::tooling::runToolOnCode(newASTFrontendAction(testTupleUnpacker),
	                              R"EOF(
	                              class C;
	                              __attribute__((annotate("pyexport")))
	                              int foo(int bar, const char *baz, const C &quux);
	                              C foo(int bar);

	                              )EOF");


	std::cout << "=========================================================================\n";

	auto testClass = [&](clang::ASTContext &ctx) {
		auto tu = ctx.getTranslationUnitDecl();
		for(auto decl : streams::stream(tu->decls_begin(), tu->decls_end()))
		{
			if(!hasAnnotation(*decl, "pyexport")) continue;

			if(auto classDecl = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(decl))
			{
				classDecl->dumpColor();

				auto klass = autobind::makeUnique<autobind::Class>(*classDecl);
				klass->codegenDeclaration(std::cout);
				klass->codegenDefinition(std::cout);
			}
		}
	};


	auto classSample = R"EOF(
	class __attribute__((annotate("pyexport"))) Foo
	{
	public:
		void bar() { }
	};
	)EOF";
	
	clang::tooling::runToolOnCode(newASTFrontendAction(testClass), classSample);

}


