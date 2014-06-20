#include <iostream>
#include "util.hpp"


int main()
{
	const char *s = R"EOF(
   	hello there
   	how are you?
	)EOF";

	std::cout << autobind::dedent(s) << std::endl;
}

