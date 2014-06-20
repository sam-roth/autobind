#pragma once

#include <boost/optional.hpp>
#include <map>
#include <string>
#include <sstream>


namespace autobind {
template <class T>
struct IterRange
{
	T first, last;

	T begin() const { return first; }
	T end() const { return last; }
};

template <class T>
IterRange<T> toRange(T first, T last)
{
	return {first, last};
}

#define PROP_RANGE(prefix) toRange(prefix ## _begin(), prefix ## _end())


template <class K, class V>
boost::optional<const V &> get(const std::map<K, V> &map,
                               const K &key)
{
	auto it = map.find(key);
	if(it == map.end())
	{
		return {};
	}
	else
	{
		return it->second;
	}
}

template <typename T>
struct Streamable
{
	std::string toString() const
	{
		std::stringstream ss;
		static_cast<const T &>(*this).write(ss);
		return ss.str();
	}
};


template <typename T>
std::ostream &operator <<(std::ostream &os, const Streamable<T> &s)
{
	static_cast<const T &>(s).write(os);
	return os;
}


inline std::pair<std::string, std::string> rsplit(const std::string &s, const char *delim)
{
	size_t index = s.rfind(delim);

	if(index == std::string::npos)
		return {{}, s};
	else
		return {s.substr(0, index), s.substr(index + std::strlen(delim))};
}

inline void replace(std::string &str, const char *pat, const char *repl)
{
	const size_t patLen = std::strlen(pat);

	for(size_t index = str.find(pat); index != str.npos; index = str.find(pat, index))
	{
		str.replace(index, patLen, repl);
	}
}

std::string dedent(const std::string &s);

} // autobind