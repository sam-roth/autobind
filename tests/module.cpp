// This file should compile successfully.

#include <autobind.hpp>

pymodule(module);


#define GETTER(x) pygetter(x) decltype(_##x) x() const { return _##x; }


struct pyexport Keywords
{
	int _foo;
	std::string _bar;
	std::string _baz;

	Keywords(int foo,
	         const char *bar,
	         const std::string &baz)
	: _foo(foo)
	, _bar(bar)
	, _baz(baz)
	{ }

	GETTER(foo)
	GETTER(bar)
	GETTER(baz)
};


struct pyexport Accessors
{
	Accessors(int foo)
	: foo(foo) { }

	/// docstring for foo
	int pyexport foo;
};


struct pyexport Methods
{
	std::string s;

	Methods(std::string s)
	: s(s) { }


	std::string foo() const
	{
		return "foo" + s;
	}

	std::string bar() const
	{
		return "bar" + s;
	}
};


pyexport Keywords make_keywords(int foo, const char *bar, std::string baz)
{
	return {foo, bar, baz};
}

static int allocs = 0, deallocs = 0;

pyexport void reset_allocs() { allocs = deallocs = 0; }
pyexport int alloc_count() { return allocs; }
pyexport int dealloc_count() { return deallocs; }

struct pyexport AllocCheck
{
	AllocCheck()
	{
		++allocs;
	}

	AllocCheck(const AllocCheck &)
	{
		++allocs;
	}

	AllocCheck &operator =(const AllocCheck &) { return *this; }

	AllocCheck(AllocCheck &&) = delete;
	AllocCheck &&operator =(AllocCheck &&) = delete;

	~AllocCheck()
	{
		++deallocs;
	}
};


struct pyexport ThrowCheck
{
	AllocCheck ac;

	ThrowCheck()
	{
		if(std::rand() % 2 == 0)
		{
			throw std::runtime_error("test");
		}
	}
};

struct pyexport ConstructorOverload
{
	int argCount;

	ConstructorOverload(): argCount(0) { }
	ConstructorOverload(int): argCount(1) { }
	ConstructorOverload(int, int): argCount(2) { }

	int get() const { return argCount; }
};

/// docstring test 1
pyexport void docstring_test_1() { }
/// docstring test 2
pyexport int docstring_test_2(int i, const std::string &j) { return 0; }
/// docstring test 3
struct pyexport DocstringTest3
{
	/// docstring test 4
	int docstring_test_4(int i) { return 0; }
};

int double_number(int x) { return x * 2; }
struct TestUsing { };
namespace pyexport
{
	using ::double_number;
	using ::TestUsing;
}


pyexport std::string overload() { return ""; }
pyexport std::string overload(int) { return "int"; }
pyexport std::string overload(const std::string &) { return "std::string"; }
pyexport std::string overload(int, int) { return "int, int"; }

