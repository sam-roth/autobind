

#ifndef FUNC_HPP_2Q8A6A
#define FUNC_HPP_2Q8A6A

#include <vector>
#include <clang/AST/Decl.h>

#include "../Export.hpp"

namespace autobind {

class Func: public Export
{
	std::vector<const clang::FunctionDecl *> _decls;
	std::string _implRef;
	std::string _selfTypeRef;

	void codegenPrototype(std::ostream &) const;
public:
	Func(std::string name);

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

	void setSelfTypeRef(std::string r)
	{
		_selfTypeRef = std::move(r);
	}
	
	const std::string &selfTypeRef() const
	{
		return _selfTypeRef;
	}

	virtual void merge(const Export &e) override;
};


} // autobind

#endif // FUNC_HPP_2Q8A6A

