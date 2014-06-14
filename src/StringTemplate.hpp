#pragma once
#include <string>
#include <ostream>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include <boost/regex.hpp>
#include "streamindent.hpp"

struct IStreamable
{
	virtual void write(std::ostream &os) const = 0;
	virtual ~IStreamable() { }
};

template <typename T>
class StreamableWrapper: public IStreamable
{
	T _object;
public:
	StreamableWrapper(const T &object)
	: _object(object) { }

	void write(std::ostream &os) const final override
	{
		os << _object;
	}
};

template <typename T>
std::unique_ptr<IStreamable> makeStreamable(const T &object)
{
	return std::unique_ptr<IStreamable>(new StreamableWrapper<const T &>(object));
}

struct ITemplateNamespace
{
	virtual const IStreamable &get(const std::string &key) const = 0;
	virtual ~ITemplateNamespace() { }
};

namespace {
	boost::regex templateRegex("\\{\\{(.*?)\\}\\}",
	                           boost::regex::ECMAScript);
}

class SimpleTemplateNamespace: public ITemplateNamespace
{
	std::unordered_map<std::string, std::unique_ptr<IStreamable>> _data;
public:
	const IStreamable &get(const std::string &key) const final override
	{
		return *_data.at(key);
	}

	template <typename T>
	SimpleTemplateNamespace &set(const std::string &key,
	                             const T &value)
	{
		_data[key] = makeStreamable(value);
		return *this;
	}
};



class StringTemplate
{
	std::string _fmt;
public:
	StringTemplate(const char *format)
	: _fmt(format) { }
	StringTemplate(const std::string &format)
	: _fmt(format)
	{
		
	}

	void expand(std::ostream &os, const ITemplateNamespace &ns) const
	{

		auto replacer = [&](const boost::smatch &m) {
			std::stringstream ss;
			ns.get(m[1]).write(ss);
			return ss.str();
		};

		auto s = boost::regex_replace(_fmt,
		                              templateRegex,
		                              replacer);


		os << s;

	}
};



