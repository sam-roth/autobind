#include <iostream>
#include "util.hpp"
#include "StringTemplate.hpp"

int main()
{
	const char *s = R"EOF(
   	hello there
   	how are you?
	)EOF";


	autobind::StringTemplate stmp = R"EOF(

	a{{b}}

	)EOF";



	stmp.into(std::cout)
		.set("b", "")
		.expand();

}


