// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef REGEX_HPP_CKX3T4
#define REGEX_HPP_CKX3T4
#include <regex>
#include <string>


namespace autobind
{

template <class Func>
std::string regex_replace(const std::string &s,
                          const std::regex &pattern,
                          Func f)
{
	typedef std::string::const_iterator It;
	typedef std::regex_iterator<It> RegexIt;

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





