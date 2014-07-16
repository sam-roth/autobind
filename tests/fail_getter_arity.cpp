// Expected
// ========
//
// …/tests/fail_getter_arity.cpp:15:23: error: getter must have no parameters
//         int pygetter(s) s(int) { return 0; }
//                              ^
// 1 error generated.

#include <autobind.hpp>

pymodule(fail_getter_arity);

struct pyexport C
{
	int pygetter(s) s(int) { return 0; }
};

