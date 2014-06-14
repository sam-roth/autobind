
#include "autobind.hpp"

pymodule(example);

template <class T>
void printEach(const std::vector<T> &items)
{
	for(const auto &item : items)
	{
		std::cout << item << std::endl;
	}
}

// extern template
// pyexport void printEach<int>(const std::vector<int> &items);

// template <>
// pyexport void printEach<int>(const std::vector<int> &items);

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

