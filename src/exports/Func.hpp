

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
protected:
	virtual void codegenPrototype(std::ostream &) const;
	virtual void codegenOverload(std::ostream &, size_t) const;
	virtual void beforeOverloads(std::ostream &) const { }
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

	const std::string &implRef() const
	{
		return _implRef;
	}

	virtual void merge(const Export &e) override;
	virtual ~Func() { }
};



class Constructor: public Func
{
	bool _defaultConstructible = false;
	void codegenOverloadOrDefault(std::ostream &, int) const;
public:
	Constructor(std::string name)
	: Func(name) { }

	void setDefaultConstructible()
	{
		_defaultConstructible = true;
	}
protected:
	virtual void codegenPrototype(std::ostream &) const;
	virtual void codegenOverload(std::ostream &, size_t) const;
	virtual void beforeOverloads(std::ostream &) const;
};


} // autobind

#endif // FUNC_HPP_2Q8A6A

