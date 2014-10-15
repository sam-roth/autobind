

#include <autobind.hpp>

pymodule(fail_no_conversion);

class Foo
{
};

pyexport Foo makeFoo()
{
	return {};
}

pyexport void bar(const Foo &f)
{
}


