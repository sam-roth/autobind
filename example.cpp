

#include "autobind.hpp"

pymodule     (example);
pydocstring  ("example functions to be consumed by python");


struct pyexport TestStruct
{
	TestStruct()
	{
		std::cout << "constructed " << this << "\n";
	}

	~TestStruct()
	{
		std::cout << "destructed " << this << "\n";
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
