#include <autobind.hpp>

pymodule(fail_no_conversion2);
struct Foo{};

struct pyexport Bar
{
	Foo pyexport f;
};

