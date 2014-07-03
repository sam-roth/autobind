// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef REGEX_HPP_CKX3T4
#define REGEX_HPP_CKX3T4

#ifdef AB_NO_STD_REGEX // for GCC 4.8, which doesn't have std::regex.
#	include <boost/tr1/regex.hpp>
#else
#	include <regex>
#endif // AB_NO_STD_REGEX

#include <string>


namespace autobind
{
	#ifdef AB_NO_STD_REGEX
	namespace regex
	{
		namespace regex_constants = std::tr1::regex_constants;
		using std::tr1::regex_error;
		using std::tr1::regex_traits;
		using std::tr1::basic_regex;
		using std::tr1::sub_match;
		using std::tr1::match_results;
		using std::tr1::regex_match;
		using std::tr1::regex_search;
		using std::tr1::regex_replace;
		using std::tr1::regex_iterator;
		using std::tr1::regex_token_iterator;
		using std::tr1::regex;
	}
	#else
	namespace regex
	{
		namespace regex_constants = std::regex_constants;
		using std::regex_error;
		using std::regex_traits;
		using std::basic_regex;
		using std::sub_match;
		using std::match_results;
		using std::regex_match;
		using std::regex_search;
		using std::regex_replace;
		using std::regex_iterator;
		using std::regex_token_iterator;
		using std::regex;
	}
	#endif
template <class Func>
std::string regex_replace(const std::string &s,
                          const std::regex &pattern,
                          Func f)
{
	typedef std::string::const_iterator It;
	typedef regex::regex_iterator<It> RegexIt;

	auto it = RegexIt(s.begin(), s.end(), pattern);
	auto end = RegexIt();

	auto lastSuffix = s.begin();

	std::string result;

	for(; it != end; ++it)
	{
		auto prefix = it->prefix();
		result.insert(result.end(), prefix.first, prefix.second);
		result += f(*it);
		lastSuffix = it->suffix().first;
	}
	
	result.insert(result.end(), lastSuffix, s.end());

	return result;
}

} // autobind

#endif // REGEX_HPP_CKX3T4





