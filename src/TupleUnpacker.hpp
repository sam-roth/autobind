#ifndef TUPLEUNPACKER_HPP_8BN4HT
#define TUPLEUNPACKER_HPP_8BN4HT
#include <string>
#include <clang/AST/Decl.h>
#include "util.hpp"
namespace autobind {

/// Generates code for unpacking a Python tuple.
class TupleUnpacker
{
	struct StorageElt
	{
		std::string type, name;
		std::string msg;
		std::string realType;
	};

	const std::string _argsRef, _kwargsRef;
	const std::string _okRef;
	std::string _format;
	std::vector<StorageElt> _storageElements;
	std::vector<std::string> _elementRefs;
	std::vector<std::string> _argNames;
public:
	TupleUnpacker(std::string argsRef,
	              std::string kwargsRef)
	: _argsRef(std::move(argsRef))
	, _kwargsRef(std::move(kwargsRef))
	, _okRef(gensym("ok"))
	{ }


	void addElement(const clang::VarDecl &decl);

	/// Get expressions that will refer to the unpacked elements of the tuple.
	const std::vector<std::string> &elementRefs() const;

	/// Get the name of the boolean flag used to indicate whether unpacking was successful.
	const std::string &okRef() const;

	/// Generate the code for the tuple unpack.
	void codegen(std::ostream &) const;
};


} // autobind



#endif // TUPLEUNPACKER_HPP_8BN4HT

