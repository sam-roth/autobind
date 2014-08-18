


#include <autobind.hpp>

pymodule(ok_conversion);

class pyexport Foo
{
};

pyexport Foo makeFoo()
{
	return {};
}

pyexport void bar(const Foo &f)
{
}

