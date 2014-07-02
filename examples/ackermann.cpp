

#include <autobind.hpp>

pymodule(ackermann);

/// Compute the value of A(m, n), the Ackermann function.
int pyexport ackermann(int m, int n) {
	if(m < 0 || n < 0) throw std::out_of_range("m and n must be non-negative");
	return m == 0? n + 1 
	:      n == 0? ackermann(m - 1, 1)
	:              ackermann(m - 1, ackermann(m, n - 1));
}





