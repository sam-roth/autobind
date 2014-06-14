#ifndef PRINTING_HPP_ETTWFK
#define PRINTING_HPP_ETTWFK

#include <ostream>
#include <sstream>
#include "stream.hpp"

namespace streams {


namespace detail 
{

	struct ToString
	{
		template <class T>
		std::string operator()(const T &what) const
		{
			std::stringstream ss;
			ss << what;
			return ss.str();
		}
	};

	template <class Stream>
	struct ConcatStreamer
	{
		Stream s;
		std::string toString() const;
	};

	template <class Stream>
	std::ostream &operator <<(std::ostream &os,
	                          ConcatStreamer<Stream> s)
	{
		for(const auto &entry : s.s)
		{
			os << entry;
		}

		return os;
	}

	template <class Stream>
	std::string ConcatStreamer<Stream>::toString() const
	{
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}
}

namespace { detail::ToString str __attribute__((__unused__)); }



template <
	class T,
	class=typename std::enable_if<
		IsStream<T>::value
	>::type
>
detail::ConcatStreamer<T> cat(T stream)
{
	return {std::move(stream)};
}

} // namespace streams

#endif // PRINTING_HPP_ETTWFK