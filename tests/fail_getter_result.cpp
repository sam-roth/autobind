// Expected
// ========
//
// …/tests/fail_getter_result.cpp:15:19: error: getter must not return `void`.
//         void pygetter(s) s() { }
//                              ^
// 1 error generated.

#include <autobind.hpp>

pymodule(fail_getter_result);

struct pyexport C
{
	void pygetter(s) s() { }
};

