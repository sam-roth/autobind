

#ifndef DATA_HPP_F6Z1XF
#define DATA_HPP_F6Z1XF

#include <memory>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>
#include <unordered_map>
#include <boost/format.hpp>
#include "stream.hpp"
#include "streamindent.hpp"

namespace autobind {

std::string gensym(const std::string &prefix="G");
std::string processDocString(const std::string &docstring);

class Module;

class Export
{
	std::string _name;
public:
	Export(std::string name)
	: _name(std::move(name)) { }
	virtual ~Export() { }

	const std::string &name() const { return _name; }

	virtual void setModule(Module &) { }

	virtual void codegenDeclaration(std::ostream &) const = 0;
	virtual void codegenDefinition(std::ostream &) const = 0;
	virtual void codegenMethodTable(std::ostream &) const = 0;
	virtual void codegenInit(std::ostream &) const { }

};


} // namespace autobind

#endif // DATA_HPP_F6Z1XF

