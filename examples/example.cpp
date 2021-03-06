

#include <autobind.hpp>
#include <map>

namespace python = autobind::python;

pymodule     (example);
pydocstring  ("example functions to be consumed by python");

/// Print a lovely skit to standard output.
///
/// Keyword argument example from Python docs:
/// https://docs.python.org/3.3/extending/extending.html
///
/// Default arguments aren't supported yet.
pyexport void kwarg_parrot(int voltage,
                           const char *state,
                           const char *action,
                           const char *type)
{
	std::cout << "-- This parrot wouldn't " << action << " if you put " << voltage << " Volts through it.\n";
	std::cout << "-- Lovely plumage, the " << type << " -- It's " << state << "\n";
}




struct pyexport TestStruct
{
	TestStruct(int integer)
	{
		std::cout << "constructed " << this << "\n";
	}

	void test()
	{
		std::cout << "called test() " << this << "\n";
	}

	~TestStruct()
	{
		std::cout << "destructed " << this << "\n";
	}
private:
	void test2() { }
};

/// A homogenous, sorted dictionary mapping str -> str.
struct pyexport StringMap
{
	std::map<std::string, std::string> m;

	void set(const std::string &lhs, const std::string &rhs)
	{
		m[lhs] = rhs;
	}

	std::string get(const std::string &key) const
	{
		return m.at(key);
	}


	std::string repr() const
	{
		bool first = true;
		std::string result = "{";
		for(const auto &item : m)
		{
			if(first)
			{
				first = false;
			}
			else
			{
				result += ", ";
			}

			result += item.first + ": " + item.second;
		}
		result += "}";

		return result;
	}
};

pyexport std::string tryConvertingToInt(python::ObjectRef o)
{
	try
	{
		int i = o.convert<int>();
		return std::to_string(i);
	}
	catch(python::Exception &exc)
	{
		std::cout << exc.what() << std::endl;
		throw;
	}
}


/// run a program
pyexport int system(const char *);

/// throw an exception
pyexport void foo(const std::string &s)
{
	throw std::runtime_error("Foo is merely a metasyntactic variable. You must never invoke it, "
	                         "especially with, \"" + s + "\".");
}



/// double each item in the list in place
pyexport void doubleEach(python::ListRef list)
{
	size_t len = list.size();

	for(size_t i = 0; i < len; ++i)
	{
		list.set(i, list.get<int>(i) * 2);
	}
}

/// return a list of the items from the collection, doubled.
pyexport std::vector<int> doubled(const std::vector<int> &ints)
{
	std::vector<int> v;
	for(int i : ints)
	{
		v.push_back(i * 2);
	}

	return v;
}

// declare a getter/setter pair
#define ACCESSORS(f) \
	pygetter(f) decltype(_##f) get_ ## f() const { return _##f; }\
	void pysetter(f) set_ ## f(decltype(_##f) value) { _##f = value; }


/// Noddy example from the Python documentation:
/// https://docs.python.org/3.3/extending/newtypes.html
class pyexport Noddy
{
	std::string _first, _last;
	int _number;

public:
	ACCESSORS(number) // Because Autobind is compiler-based, it can see straight through macros
	                  // and decltype.
	ACCESSORS(first)
	ACCESSORS(last)

	pygetter(name)
	std::string name() const
	{
		return _first + " " + _last;
	}

};

template <>
struct python::protocols::Str<Noddy>
{
	static std::string convert(const Noddy &n)
	{
		return "Noddy(" + n.name() + ")";
	}
};

pyexport void callWithCount(python::ObjectRef callable)
{
	static int count = 0;
	callable(count++);
}

pyexport void callMethodWithCount(python::ObjectRef obj, const char *method)
{
	static int count = 0;
	getattr(obj, method)(count++);
}
