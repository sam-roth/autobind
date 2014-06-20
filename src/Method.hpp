#ifndef METHOD_HPP_69APQA
#define METHOD_HPP_69APQA

#include "Export.hpp"

namespace autobind {


class Method: public Function
{
	std::string _selfTypeName;
public:

	using Function::Function;

	void setSelfTypeName(const std::string &s)
	{
		_selfTypeName = s;
	}

	virtual const char *selfTypeName() const { return _selfTypeName.c_str(); }
	virtual void codegenCall(std::ostream &) const override;
};

} // autobind

#endif // METHOD_HPP_69APQA

