

#ifndef FUNC_HPP_2Q8A6A
#define FUNC_HPP_2Q8A6A

#include <vector>
#include <clang/AST/Decl.h>

#include "../Export.hpp"

namespace autobind {

class Func: public Export
{
	std::vector<const clang::FunctionDecl *> _decls;
public:
	using Export::Export;

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;

	void addDecl(const clang::FunctionDecl &decl) 
	{
		_decls.push_back(&decl);
	}

	const std::vector<const clang::FunctionDecl *> &decls() const
	{
		return _decls;
	}

	virtual void merge(const Export &e) override;
};


} // autobind

#endif // FUNC_HPP_2Q8A6A

