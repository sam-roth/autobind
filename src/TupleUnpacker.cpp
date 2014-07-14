#include "TupleUnpacker.hpp"
#include "StringTemplate.hpp"
#include "stream.hpp"
#include "printing.hpp"

#include <clang/AST/ASTContext.h>

namespace autobind {

void TupleUnpacker::addElement(const clang::VarDecl &decl)
{
	auto &ctx = decl.getASTContext();
	auto constCharStarTy = ctx.getPointerType(ctx.getConstType(ctx.CharTy)).getTypePtr();
	auto intTy = ctx.IntTy.getTypePtr();

	auto unqualQType = decl
		.getType()
		.getCanonicalType()
		.getNonReferenceType()
		.getUnqualifiedType();
	auto unqualType = unqualQType
		.getTypePtr();

	auto argIdent = gensym(decl.getName());
	_argNames.push_back(decl.getName());

	StorageElt elt;

	if(unqualType == constCharStarTy)
	{
		_format += "s";
		elt.type = "const char *";
		elt.name = argIdent;
		_elementRefs.push_back(argIdent);
	}
	else if(unqualType == intTy)
	{
		_format += "i";
		elt.type = "int";
		elt.name = argIdent;
		_elementRefs.push_back(argIdent);
	}
	else
	{
		_format += "O&";
		elt.type = "::python::detail::ConversionFunc<"
		                              + unqualQType.getAsString() + ">::Value";
		elt.name = argIdent;
		elt.msg = ("expected object convertible to " + unqualQType.getAsString() + " for argument "
		           + std::to_string(_storageElements.size() + 1) + " ("
		           + decl.getName() + ")").str();
		elt.realType = unqualQType.getAsString();
		_elementRefs.push_back("*" + argIdent + ".value");
	}

	_storageElements.push_back(elt);
}


const std::vector<std::string> &TupleUnpacker::elementRefs() const
{
	return _elementRefs;
}


const std::string &TupleUnpacker::okRef() const
{
	return _okRef;
}


void TupleUnpacker::codegen(std::ostream &out) const
{
	using namespace streams;

	static const StringTemplate top = R"EOF(
	
	static const char *{{kwlist}}[] = {
		{{argnames}}
		0
	};

	{{decls}}

	int {{ok}} = PyArg_ParseTupleAndKeywords(
		{{args}},
		{{kw}}, 
		"{{fmt}}",
		(char **){{kwlist}}
		{{elements}}
	);

	)EOF";

	auto kwlistSym = gensym("kwlist");

	auto argnames = stream(_argNames)
		| transformed([&](const std::string &n) {
			return "\"" + n + "\", ";
		});

	auto decls = stream(_storageElements)
		| transformed([&](const StorageElt &e) {
			auto result = e.type + " " + e.name + ";\n";
			if(!e.msg.empty())
			{
				// We control this string so we don't need to worry about escaping it here.
				result += e.name + ".errorMessage = \"" + e.msg + "\";\n"; 
			}

			return result;
		});

	auto elements = stream(_storageElements)
		| transformed([&](const StorageElt &e) {
			if(!e.realType.empty()) // this could be done more elegantly
			{
				return ",\n &::python::detail::ConversionFunc<" + e.realType + ">::convert, &" + e.name;
			}
			else
			{
				return ",\n &" + e.name;
			}
		});

	top.into(out)
		.set("kwlist", kwlistSym)
		.set("argnames", cat(argnames))
		.set("decls", cat(decls))
		.set("ok", _okRef)
		.set("args", _argsRef)
		.set("kw", _kwargsRef)
		.set("kwlist", kwlistSym)
		.set("fmt", _format)
		.set("elements", cat(elements))
		.expand();
}


} // autobind


