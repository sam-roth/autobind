#pragma once

#include <boost/optional.hpp>
#include <map>
#include <string>
#include <sstream>
#include <memory>
#include <clang/AST/ASTConsumer.h>
/// Defines a function with the signature `signature` and an automatcally-deduced return type,
/// returning the given expression. The `expression` argument must be an expression (no semicolon, if, ...).
/// Example: AB_RETURN_AUTO(doubleNumber(int n), n * 2)
///
/// This was added because the code was backported from C++1y to C++11.
///
#define AB_RETURN_AUTO(signature, expression)\
	auto signature -> decltype(expression) { return (expression); }

template <class F>
clang::ASTConsumer *newASTConsumer(F func)
{
	struct ResultType: public clang::ASTConsumer
	{
		F _func;
		ResultType(F func): _func(std::move(func)) { }

		void HandleTranslationUnit(clang::ASTContext &context) final override
		{
			_func(context);
		}
	};

	return new ResultType(std::move(func));
}


namespace autobind {


template <class T, class F>
class MethodRef
{
	T &&that;
	F mbr;
public:
	MethodRef(T &&that, F mbr)
	: that(that)
	, mbr(mbr)
	{

	}

	template <class... Args>
	AB_RETURN_AUTO(operator()(Args &&... args) const,
	               (that.*mbr)(std::forward<Args>(args)...))

};

template <class T, class F>
MethodRef<T, F> method(T &&that, F mbr)
{
	return MethodRef<T, F>(that, mbr);
}


template <class T, class... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::string gensym(const std::string &prefix="G");
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