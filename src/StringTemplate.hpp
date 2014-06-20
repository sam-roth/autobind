#pragma once
#include <string>
#include <ostream>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include <boost/regex.hpp>
#include "streamindent.hpp"
#include "util.hpp"

namespace autobind {

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

template <typename F>
std::unique_ptr<IStreamable> functionStreamable(F function)
{
	class FunctionStreamable: public IStreamable
	{
		F _func;
	public:
		FunctionStreamable(F func): _func(std::move(func)) {}

		void write(std::ostream &os) const final override
		{
			_func(os);
		}
	};

	return std::unique_ptr<IStreamable>(new FunctionStreamable(std::move(function)));
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

	template <typename F>
	SimpleTemplateNamespace &setFunc(const std::string &key,
	                                 F func)
	{
		_data[key] = functionStreamable(std::move(func));
		return *this;
	}
};



class StringTemplate
{
	std::string _fmt;
public:
	StringTemplate(const char *format)
	: _fmt(dedent(format)) { }
	StringTemplate(const std::string &format)
	: _fmt(dedent(format))
	{
		
	}

	void expand(std::ostream &os, const ITemplateNamespace &ns) const
	{

		auto replacer = [&](const boost::smatch &m) {

			bool needsIndent = true;
			int index = m.position() - 1;
			for(; index >= 0; --index)
			{
				if(_fmt[index] == '\n') 
				{
					break;
				}
				else if(!std::isspace(_fmt[index]))
				{
					needsIndent = false;
					break;
				}
			}

			++index;

			std::stringstream ss;

			if(needsIndent && index != m.position())
			{
				IndentingOStreambuf indenter(ss, std::string(_fmt.begin() + index, _fmt.begin() + m.position()), false);
				ns.get(m[1]).write(ss);
			}
			else
			{
				ns.get(m[1]).write(ss);
			}
			
			return ss.str();
		};

		auto s = boost::regex_replace(_fmt,
		                              templateRegex,
		                              replacer);


		os << s;

	}
};


} // autobind
