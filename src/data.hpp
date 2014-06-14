

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

class Module;

class Export
{
	std::string _name;
public:
	Export(std::string name)
	: _name(std::move(name)) { }
	virtual ~Export() { }

	const std::string &name() const { return _name; }

	virtual void codegenDeclaration(std::ostream &) const = 0;
	virtual void codegenDefinition(std::ostream &) const = 0;
	virtual void codegenMethodTable(std::ostream &) const = 0;

};

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

	virtual void codegenDeclaration(std::ostream &) const override;
	virtual void codegenDefinition(std::ostream &) const override;
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
private:
	std::vector<Arg> _args;
	std::string _implName, _unqualifiedName, _docstring, _origFile, _returnType;
	int _lineNo = -1;
};

class Module
{
	std::string _name;
	std::string _sourceTUPath;
	std::vector<std::unique_ptr<Export>> _exports;
public:

	void addExport(std::unique_ptr<Export> e)
	{
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

	void codegenDeclaration(std::ostream &out) const
	{
		assert(!_sourceTUPath.empty());

		out << "#include <Python.h>\n";
		out << boost::format("#include \"%s\"\n") % _sourceTUPath;

		for(const auto &e : _exports)
		{
			e->codegenDeclaration(out);
		}

	}

	void codegenDefinition(std::ostream &out) const
	{
		for(const auto &e : _exports)
		{
			e->codegenDefinition(out);
		}
	}

	void codegenMethodTable(std::ostream &out) const
	{
		out << "static PyMethodDef methods[] = {\n";
		{
			IndentingOStreambuf indenter(out);
			for(const auto &e : _exports)
			{
				e->codegenMethodTable(out);
			}

			out << "{0, 0, 0, 0}\n";
		}
		
		out << "};\n";
	}

	void codegenModuleDef(std::ostream &out) const
	{
		out << "static struct PyModuleDef module = {\n";
		{
			IndentingOStreambuf indenter(out);

			out  
				<< "PyModuleDef_HEAD_INIT,\n"
				<< "\"" << name() << "\",\n"
				<< "0,\n"
				<< "-1,\n"
				<< "methods\n";
		}
		out << "};\n";
	}

	void codegenInit(std::ostream &out) const
	{
		out
			<< "PyMODINIT_FUNC PyInit_" << name() << "()\n"
			<< "{\n"
			<< "    return PyModule_Create(&module);\n"
			<< "}\n";
	}

	void codegen(std::ostream &out) const
	{
		codegenDeclaration(out);
		codegenDefinition(out);
		codegenMethodTable(out);
		codegenModuleDef(out);
		codegenInit(out);
	}

};

class ModuleManager
{
	std::unordered_map<std::string, Module> _modules;

public:
	Module &module(const std::string &name)
	{
		auto it = _modules.find(name);
		if(it == _modules.end())
		{
			auto &result = _modules[name];
			result.setName(name);
			return result;
		}
		else
		{
			return it->second;
		}
	}

	auto moduleStream() const
	{
		return streams::stream(_modules);
	}

	
	void codegen(std::ostream &out) const
	{
		for(const auto &item : _modules)
		{
			item.second.codegen(out);
		}
	}

};

} // namespace autobind

#endif // DATA_HPP_F6Z1XF

