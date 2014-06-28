
#include "util.hpp"
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <unordered_map>


namespace autobind {
std::unordered_map<std::string, size_t> gensyms;

inline std::string symbolify(const std::string &name)
{
	static const boost::regex pat(R"([^A-Za-z0-9]+)");
	return boost::regex_replace(name, pat, "_");
}

std::string gensym(const std::string &prefixUnsymbolified)
{
	auto prefix = symbolify(prefixUnsymbolified);

	auto it = gensyms.find(prefix);
	if(it == gensyms.end())
	{
		gensyms[prefix] = 1;
		return prefix + "$0";
	}
	else
	{
		size_t oldCount = it->second;
		++it->second;
		return prefix + "$" + std::to_string(oldCount);
	}
}

// Translated from the original Python: http://hg.python.org/cpython/file/2.7/Lib/textwrap.py
// Comments included verbatim.
std::string dedent(const std::string &inputString)
{

	auto str = inputString;

	static boost::regex leadingWhitespace("(^[ \t]*)(?:[^ \t\n])");
	static boost::regex whitespaceOnly("^[ \t]+$");

	str = boost::regex_replace(str, whitespaceOnly, "");
	boost::match_results<std::string::const_iterator> indents;
	boost::regex_search(str, indents, leadingWhitespace);

	boost::optional<std::string> margin;

	// Look for the longest leading string of spaces and tabs common to
	// all lines
	for(const auto &indentMatch : indents)
	{
		auto indent = indentMatch.str();

		if(!margin)
		{
			margin = indent;
		}
		// Current line more deeply indented than previous winner:
		// no change (previous winner is still on top).
		else if(boost::starts_with(indent, *margin)) { }
		// Current line consistent with and no deeper than previous winner:
		// it's the new winner.
		else if(boost::starts_with(*margin, indent))
		{
			margin = indent;
		}
		// Current line and previous winner have no common whitespace:
		// there is no margin.
		else
		{
			margin = "";
			break;
		}
	}

	if(margin && !margin->empty())
	{
		boost::regex rgx("^" + *margin);
		str = boost::regex_replace(str, rgx, "");
	}

	return str;
}

} // autobind


