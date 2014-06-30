
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
		std::string typeNameSpelling; ///< actual type name used by client code
	};

	typedef std::vector<Arg> Args;

	struct Signature
	{
		Args args;
		std::string cppReturnTypeName, returnTypeSpelling;
	};

	typedef std::vector<Signature> Signatures;

	Function(std::string name,
	         std::vector<Arg> args,
	         std::string docstring={});

	const Signatures &signatures() const { return _signatures; }
	const Signature &signature(size_t index) const { return _signatures.at(index); }
	size_t signatureCount() const { return _signatures.size(); }


	const std::string &implName() const { return _implName; }
	const std::string &unqualifiedName() const { return _unqualifiedName; }
	const std::string &docstring() const { return _docstring; }

	virtual const char *selfTypeName() const { return _selfTypeName.c_str(); }

	void setSelfTypeName(const std::string &s)
	{
		_selfTypeName = s;
	}

	virtual void codegenCall(std::ostream &, size_t index) const;
	virtual void codegenCallArgs(std::ostream &, size_t index) const;
	virtual void codegenTupleUnpack(std::ostream &, size_t index) const;

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenDefinitionBody(std::ostream &, size_t index) const;
	virtual void codegenMethodTable(std::ostream &) const override;

	void setSourceLocation(int lineNo, const std::string &origFile)
	{
		_lineNo = lineNo;
		_origFile = origFile;
	}

	void setReturnType(const std::string &name,
	                   const std::string &spelling="")
	{
		assert(!_signatures.empty());

		auto &sig = _signatures[0];
		if(spelling.empty())
		{
			sig.cppReturnTypeName = name;
			sig.returnTypeSpelling = name;
		}
		else
		{
			sig.cppReturnTypeName = name;
			sig.returnTypeSpelling = spelling;
		}
	}

	void setPythonName(const std::string &s)
	{
		_pythonName = s;
	}

	const std::string &pythonName() const
	{
		return _pythonName.empty()? unqualifiedName() : _pythonName;
	}

	bool isMethod() const { return _isMethod; }
	void setMethod() { _isMethod = true; }


	virtual void merge(const Export &e) override;

private:
	Signatures _signatures;

	std::string _implName, _unqualifiedName, _docstring, 
		_origFile, _pythonName, _selfTypeName;
	int _lineNo = -1;
	bool _isMethod = false;
};


} // autobind


#endif // FUNCTION_HPP_TEKGJC



