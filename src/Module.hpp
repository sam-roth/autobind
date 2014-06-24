#ifndef MODULE_HPP_FN116U
#define MODULE_HPP_FN116U
#include "Export.hpp"

namespace autobind {

class Module
{
	std::string _name;
	std::string _sourceTUPath;
	std::string _docstring;
	std::vector<std::unique_ptr<Export>> _exports;
public:


	void setDocstring(const std::string &docstring)
	{
		_docstring = docstring;
	}

	void addExport(std::unique_ptr<Export> e)
	{
		e->setModule(*this);
		_exports.push_back(std::move(e));

	}

	const std::vector<std::unique_ptr<Export>> &exports() const
	{
		return _exports;
	}

	const std::string &name() const
	{
		return _name;
	}

	void setName(const std::string &name)
	{
		_name = name;
	}

	void setSourceTUPath(const std::string &path)
	{
		_sourceTUPath = path;
	}

	void codegenDeclaration(std::ostream &out) const;
	void codegenDefinition(std::ostream &out) const;
	void codegenMethodTable(std::ostream &out) const;
	void codegenModuleDef(std::ostream &out) const;
	void codegenInit(std::ostream &out) const;
	void codegen(std::ostream &out) const;

};


} // autobind

#endif // MODULE_HPP_FN116U