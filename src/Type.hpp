
#include "Export.hpp"

namespace autobind {
class Method;

class Type: public Export
{
	std::string _moduleName, _cppQualTypeName, _docstring, _structName;
	mutable std::unique_ptr<Function> _constructor;
	std::vector<std::unique_ptr<Method>> _methods;
	bool _copyAvailable = false;
public:
	Type(std::string name,
	     std::string cppQualTypeName,
	     std::string docstring);


	Function &constructor() const;
	
	void setConstructor(std::unique_ptr<Function> func);
	void addMethod(std::unique_ptr<Method> method);
	void setCopyAvailable(bool available=true) { _copyAvailable = available; }

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
	virtual void codegenMethodTable(std::ostream &) const override;

	virtual void setModule(Module &m) override;
	virtual void codegenInit(std::ostream &) const override;
};


} // autobind


