#ifndef CALLGENERATOR_HPP_APGJHN
#define CALLGENERATOR_HPP_APGJHN

#include <clang/AST/Decl.h>
#include "TupleUnpacker.hpp"

namespace autobind {

class CallGenerator
{
	TupleUnpacker _unpacker;
	const clang::FunctionDecl *const _decl;
protected:
	virtual void codegenSuccess(std::ostream &) const;
	virtual void codegenErrorReturn(std::ostream &) const;
	virtual void codegenPythonException(std::ostream &) const;
	virtual void codegenStdException(std::ostream &) const;
public:
	CallGenerator(std::string argsRef,
	              std::string kwargsRef,
	              const clang::FunctionDecl *decl);

	void codegen(std::ostream &) const;

	const std::string &okRef() const
	{
		return _unpacker.okRef();
	}

};


} // autobind


#endif // CALLGENERATOR_HPP_APGJHN

