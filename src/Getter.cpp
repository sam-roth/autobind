
#include "Getter.hpp"
#include "StringTemplate.hpp"

namespace autobind {

namespace
{
	const boost::format FunctionPrototype{
		"static PyObject *%s(%s *self, void *)"
	};


}

Getter::Getter(std::string name, std::vector<Arg> args, std::string doc)
: autobind::Function(std::move(name),
                     std::move(args),
                     std::move(doc))
{
	setMethod();
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
		out << "PyObject *returnValue = 0;\n";
		codegenDefinitionBody(out, 0);
		out << "return returnValue;\n";
	}

	out << "}\n";
}

Setter::Setter(std::string name, std::vector<Arg> args, std::string doc) 
: autobind::Function(std::move(name),
                     std::move(args),
                     std::move(doc))
{
	setMethod();
}

void Setter::codegenTupleUnpack(std::ostream &out, size_t index) const
{
	assert(this->signatureCount() == 1 && this->signature(0).args.size() == 1);
	
	static const StringTemplate template_ = R"EOF(
	auto arg0 = python::Conversion<{{type}}>::load(value);
	)EOF";

	template_.into(out)
		.set("type", signature(0).args.front().cppQualTypeName)
		.expand();
}


void Setter::codegenDeclaration(std::ostream &out) const
{
	static const StringTemplate template_ = R"EOF(
	static int {{implName}}({{selfTypeName}} *self, PyObject *value, void *closure);
	)EOF";

	template_.into(out)
		.set("implName", implName())
		.set("selfTypeName", selfTypeName())
		.expand();
}


void Setter::codegenDefinition(std::ostream &out) const
{
	static const StringTemplate template_ = R"EOF(
	static int {{implName}}({{selfTypeName}} *self, PyObject *value, void *closure)
	{
		int ok = 1;
		if(!value)
		{
			PyErr_SetString(PyExc_TypeError, "Cannot delete attribute.");
			return -1;
		}

		try
		{
			{{tupleUnpack}}
			if(ok)
			{
				self->object.{{name}}(arg0);
			}

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

	template_.into(out)
		.set("implName", implName())
		.set("selfTypeName", selfTypeName())
		.setFunc("tupleUnpack", [&](std::ostream &out) { 
			codegenTupleUnpack(out, 0);
		})
		.set("name", name())
		.expand();
}


void Setter::codegenMethodTable(std::ostream &) const { }


} // autobind



