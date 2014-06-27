#ifndef GETTER_HPP_F3YMM1
#define GETTER_HPP_F3YMM1

#include "Method.hpp"

namespace autobind {

class Getter: public Method
{
public:
	using Method::Method;

	virtual void codegenTupleUnpack(std::ostream &) const
	{
		assert(this->args().empty());
	}

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override { }
};


class Setter: public Method
{
public:
	using Method::Method;

	virtual void codegenTupleUnpack(std::ostream &) const override;
	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;
};

} // autobind

#endif // GETTER_HPP_F3YMM1


