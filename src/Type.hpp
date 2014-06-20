
#include "Export.hpp"

namespace autobind {

class Type: public Export
{
	std::string _moduleName, _cppQualTypeName, _docstring, _structName;
	mutable std::unique_ptr<Function> _constructor;

public:
	Type(std::string name,
	     std::string cppQualTypeName,
	     std::string docstring);


	Function &constructor() const;
	void setConstructor(std::unique_ptr<Function> func);


	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;

	virtual void setModule(Module &m) override;
	virtual void codegenInit(std::ostream &) const override;
};


} // autobind


