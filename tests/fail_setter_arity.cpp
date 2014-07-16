// Expected
// ========
//
// …/tests/fail_setter_arity.cpp:15:19: error: setter must have exactly one parameter
//         void pysetter(s) setS() { }
//                          ^
// 1 error generated.

#include <autobind.hpp>

pymodule(fail_setter_arity);

struct pyexport C
{
	void pysetter(s) setS() { }
};

