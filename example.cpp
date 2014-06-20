

#include "autobind.hpp"
#include <boost/format.hpp>
#include <map>

pymodule     (example);
pydocstring  ("example functions to be consumed by python");





struct pyexport TestStruct
{
	TestStruct()
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
};

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


#define ACCESSORS(f) \
	decltype(_##f) get_ ## f() const { return _##f; }\
	void set_ ## f(decltype(_##f) value) { _##f = value; }

class pyexport Noddy
{
	std::string _first, _last;
	int _number;

public:
	ACCESSORS(number)
	ACCESSORS(first)
	ACCESSORS(last)

	std::string name() const
	{
		return _first + _last;
	}

};

template <>
struct python::protocols::Str<Noddy>
{
	static std::string convert(const Noddy &n)
	{
		return "Noddy( " + n.name() + ")";
	}
};


