#ifndef GETTER_HPP_F3YMM1
#define GETTER_HPP_F3YMM1

#include "Function.hpp"

namespace autobind {

class Getter: public Function
{
public:
	Getter(std::string name,
	       std::vector<Arg> args,
	       std::string docstring={});

	virtual void codegenTupleUnpack(std::ostream &) const
	{
		assert(this->args().empty());
	}

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override { }
};


class Setter: public Function
{
public:
	Setter(std::string name,
	       std::vector<Arg> args,
	       std::string docstring={});
	
	virtual void codegenTupleUnpack(std::ostream &) const override;
	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;
};

} // autobind

#endif // GETTER_HPP_F3YMM1


