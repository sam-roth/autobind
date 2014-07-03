// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#include "util.hpp"
#include "stream.hpp"
#include "regex.hpp"
#include <boost/algorithm/string.hpp>
#include <unordered_map>


namespace autobind {
std::unordered_map<std::string, size_t> gensyms;

inline std::string symbolify(const std::string &name)
{
	static const regex::regex pat(R"([^A-Za-z0-9]+)");
	return regex::regex_replace(name, pat, std::string("_"));
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
std::string dedent(const std::string &text)
{
	boost::optional<std::string> margin;

	// Look for the longest leading string of spaces and tabs common to
	// all lines
	for(auto it = text.begin(); it != text.end(); 
	    it = std::find(it, text.end(), '\n'))
	{
		if(*it == '\n')
		{
			++it;
			if(it == text.end()) break;
		}

		auto line = it;
		while(it != text.end() && (*it == ' ' || *it == '\t')) ++it;
		if(it != text.end() && *it != '\n')
		{
			std::string indent(line, it);
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
	}

	if(!margin)
	{
		margin = "";
	}

	std::string result;
	for(auto it = text.begin(); it != text.end();)
	{
		auto next = std::find(it, text.end(), '\n');

		if(!std::all_of(it, next, [](char c) { return c==' '||c=='\t'||c=='\n'; }))
		{
			if(boost::starts_with(boost::make_iterator_range(it, next),
			                      *margin))
			{
				result += std::string(it + margin->size(), next);
			}
			else
			{
				result += std::string(it, next);
			}
		}

		if(next != text.end())
		{
			result.push_back(*next);
			++next;
		}

		it = next;
	

	}
	return result;
}

} // autobind


