
#ifndef FUNCTION_HPP_TEKGJC
#define FUNCTION_HPP_TEKGJC
#include "Export.hpp"

namespace autobind {


struct Function: public Export
{
public:
	struct Arg
	{
		char parseTupleFmt;
		bool requiresAdditionalConversion;
		std::string argName;
		std::string cppQualTypeName;
	};

	Function(std::string name,
	         std::vector<Arg> args,
	         std::string docstring={});

	const std::vector<Arg> &args() const { return _args; }
	const std::string &implName() const { return _implName; }
	const std::string &unqualifiedName() const { return _unqualifiedName; }
	const std::string &docstring() const { return _docstring; }

	virtual const char *selfTypeName() const { return "PyObject"; }


	virtual void codegenCall(std::ostream &) const;
	virtual void codegenCallArgs(std::ostream &) const;
	virtual void codegenTupleUnpack(std::ostream &) const;

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenDefinitionBody(std::ostream &) const;
	virtual void codegenMethodTable(std::ostream &) const override;

	void setSourceLocation(int lineNo, const std::string &origFile)
	{
		_lineNo = lineNo;
		_origFile = origFile;
	}

	void setReturnType(const std::string &s)
	{
		_returnType = s;
	}

	void setPythonName(const std::string &s)
	{
		_pythonName = s;
	}

	const std::string &pythonName() const
	{
		return _pythonName.empty()? unqualifiedName() : _pythonName;
	}

private:
	std::vector<Arg> _args;
	std::string _implName, _unqualifiedName, _docstring, _origFile, _returnType, _pythonName;
	int _lineNo = -1;
};

	
} // autobind


#endif // FUNCTION_HPP_TEKGJC



