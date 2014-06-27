
#include "Getter.hpp"
#include "StringTemplate.hpp"

namespace autobind {

namespace
{
	const boost::format FunctionPrototype{
		"static PyObject *%s(%s *self, void *)"
	};


}
void Getter::codegenDeclaration(std::ostream &out) const
{
	out << boost::format(FunctionPrototype) % implName() % selfTypeName() << ";\n";
}

void Getter::codegenDefinition(std::ostream &out) const
{
	out << boost::format(FunctionPrototype) % implName() % selfTypeName() << "\n{\n";

	{
		IndentingOStreambuf indenter(out);
		codegenDefinitionBody(out);
	}

	out << "}\n";
}

void Setter::codegenTupleUnpack(std::ostream &out) const
{
	assert(!this->args().empty());

	static const StringTemplate template_ = R"EOF(
	auto arg0 = python::Conversion<{{type}}>::load(value);
	)EOF";

	SimpleTemplateNamespace ns;
	ns.set("type", this->args()[0].cppQualTypeName);

	template_.expand(out, ns);
}


void Setter::codegenDeclaration(std::ostream &out) const
{
	static const StringTemplate template_ = R"EOF(
	static int {{implName}}({{selfTypeName}} *self, PyObject *value, void *closure);
	)EOF";

	SimpleTemplateNamespace ns;
	ns
		.set("implName", implName())
		.set("selfTypeName", selfTypeName());

	template_.expand(out, ns);
}


void Setter::codegenDefinition(std::ostream &out) const
{
	static const StringTemplate template_ = R"EOF(
	static int {{implName}}({{selfTypeName}} *self, PyObject *value, void *closure)
	{
		if(!value)
		{
			PyErr_SetString(PyExc_TypeError, "Cannot delete attribute.");
			return -1;
		}

		try
		{
			{{tupleUnpack}}
			self->object.{{name}}(arg0);

			return 0;
		}
		catch(python::Exception &)
		{
			return -1;
		}
		catch(std::runtime_error &exc)
		{
			PyErr_Format(PyExc_RuntimeError, "%s", exc.what());
			return -1;
		}
	}
	)EOF";

	SimpleTemplateNamespace ns;
	ns
		.set("implName", implName())
		.set("selfTypeName", selfTypeName())
		.setFunc("tupleUnpack", [&](auto &out) { 
			codegenTupleUnpack(out);
		})
		.set("name", name());

	template_.expand(out, ns);
}


void Setter::codegenMethodTable(std::ostream &) const { }


} // autobind



