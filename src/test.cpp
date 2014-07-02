#include <iostream>
#include "util.hpp"
#include "StringTemplate.hpp"
#include "regex.hpp"

static void testStringTemplate()
{
	autobind::StringTemplate stmp = R"EOF(
	b
	a{{b}}
	a
		v

			c
	)EOF";
	


	stmp.into(std::cout)
		.set("b", "")
		.expand();

}

static void testRegexReplace()
{
	const char *text = R"EOF(
		It is the programmer's responsibility to ensure that the std::basic_regex
		object passed to the iterator's constructor outlives the iterator. Because
		the iterator stores a pointer to the regex, incrementing the iterator after
		the regex was destroyed accesses a dangling pointer. If the part of the
		regular expression that matched is just an assertion (^, $, \b, \B), the
		match stored in the iterator is a zero-length match, that is,
		match[0].first == match[0].second.
	)EOF";
	const char *expected = R"EOF(
		It is the pemmargorr's rtilibisnopsey to erusne taht the std::beger_cisax
		ocejbt pessad to the iotaretr's cotcurtsnor oeviltus the iotaretr. Bsuacee
		the iotaretr serots a petnior to the regex, initnemercng the iotaretr aetfr
		the regex was deyortsed aesseccs a dnilgnag petnior. If the prat of the
		raluger eoisserpxn taht mehctad is jsut an aoitressn (^, $, \b, \B), the
		mctah serotd in the iotaretr is a zreo-ltgneh mctah, taht is,
		mctah[0].fsrit == mctah[0].snoced.
	)EOF";

	auto repl = [&](const std::smatch &m) {
		std::string mid = m[2];
		std::reverse(mid.begin(), mid.end());

		return m[1].str() + mid + m[3].str();
	};

	std::regex pattern("(\\w)(\\w+)(\\w)");

	auto munged = autobind::regex_replace(text, pattern, repl);
	assert(munged == expected);
	// for good measure
	auto reconstructed = autobind::regex_replace(munged, pattern, repl);
	assert(reconstructed == text);
}

int main()
{
	testStringTemplate();

	testRegexReplace();

}


