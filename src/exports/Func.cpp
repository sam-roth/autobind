

#include "Func.hpp"

namespace autobind {



void Func::merge(const autobind::Export &other) 
{
	if(auto that = dynamic_cast<const autobind::Func *>(&other))
	{
		for(auto decl : that->decls())
		{
			addDecl(*decl);
		}
	}
	else
	{
		Export::merge(other);
	}
}

void Func::codegenDeclaration(std::ostream &) const
{
	// TODO: implement
}


void Func::codegenDefinition(std::ostream &) const
{
	// TODO: implement
}


void Func::codegenMethodTable(std::ostream &) const
{
	// TODO: implement
}


} // autobind

