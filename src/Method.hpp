#ifndef METHOD_HPP_69APQA
#define METHOD_HPP_69APQA

#include "Function.hpp"

namespace autobind {

/// @deprecated   Use Function and setMethod() method.
class Method: public Function
{
// 	std::string _selfTypeName;
	std::string _operatorName;
public:

	Method(std::string name,
	       std::vector<Arg> args,
	       std::string docstring={})
	: Function(std::move(name), std::move(args), std::move(docstring))
	{
		setMethod();
	}


// 	void setSelfTypeName(const std::string &s)
// 	{
// 		_selfTypeName = s;
// 	}
//
	const std::string &operatorName() const { return _operatorName; }

// 	virtual const char *selfTypeName() const { return _selfTypeName.c_str(); }
// 	virtual void codegenCall(std::ostream &) const override;
	
	virtual void setOperatorName(const std::string &name)
	{
		_operatorName = name;
	}
};

} // autobind

#endif // METHOD_HPP_69APQA

