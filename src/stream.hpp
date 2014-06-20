
//// A lightweight, simple alternative to Boost.Range adaptors.


#ifndef STREAM_HPP_U2MOFE
#define STREAM_HPP_U2MOFE

#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>
#include <memory>

namespace streams {

template <class Stream>
class StreamIterator;

struct StreamBase { };

template <class Val, class Derived, class Ref=Val&>
struct Stream: public StreamBase
{
	typedef Val value_type;
	typedef Ref reference_type;

	// bool empty() const;
	// Ref  front() const;
	// void next();


	StreamIterator<Derived> begin();
	StreamIterator<Derived> end();
};


template <class T>
struct IsStream
{
	enum {
		value = std::is_base_of<StreamBase, T>::value
	};
};

template <class StreamT>
class StreamIterator: public Stream<
	std::input_iterator_tag,
	typename StreamT::value_type
>
{
	StreamT *_range = 0;
public:
	StreamIterator(StreamT &r)
	: _range(r.empty()? 0 : &r) { }

	StreamIterator()
	: _range(0) { }

	bool operator ==(const StreamIterator &other) const
	{
		return _range == other._range;
	}

	bool operator !=(const StreamIterator &other) const
	{
		return !(*this == other);
	}

	StreamIterator &operator ++()
	{
		_range->next();

		if(_range->empty()) _range = 0;

		return *this;
	}

	typename StreamT::reference_type operator *() const
	{
		return _range->front();
	}


};

template <class Val, class Derived, class Ref>
StreamIterator<Derived> Stream<Val, Derived, Ref>::begin()
{
	return StreamIterator<Derived>(*static_cast<Derived *>(this));
}

template <class Val, class Derived, class Ref>
StreamIterator<Derived> Stream<Val, Derived, Ref>::end()
{
	return StreamIterator<Derived>();
}


template <class It>
class ConstIteratorStream: public Stream<
	const typename std::iterator_traits<It>::value_type,
	ConstIteratorStream<It>,
	typename std::iterator_traits<It>::reference>
{
	It _begin, _end;
public:
	ConstIteratorStream(It begin, It end)
	: _begin(begin)
	, _end(end)
	{ }

	bool empty() const
	{
		return _begin == _end;
	}

	typename std::iterator_traits<It>::reference front() const
	{
		return *_begin;
	}

	void next()
	{
		++_begin;
	}
};


template <class Coll>
ConstIteratorStream<typename Coll::const_iterator> stream(const Coll &c)
{
	return {c.begin(), c.end()};
}

template <class It>
ConstIteratorStream<It> stream(const It &first, const It &last)
{
	return {first, last};
}

template <class SourceStream, class Transform, class Enable=void>
class TransformStream;

#define RESULT_TYPE decltype(std::declval<Transform>()(std::declval<typename SourceStream::value_type>()))
template <class SourceStream, class Transform>
class TransformStream<
	SourceStream,
	Transform,
	typename std::enable_if<
		!std::is_reference<RESULT_TYPE>::value
	>::type
>
: public Stream<
	const RESULT_TYPE,
	TransformStream<SourceStream, Transform>
>
{
	typedef RESULT_TYPE Result;

	SourceStream _source;
	Transform _transform;
	Result _result;
public:

	TransformStream(SourceStream source,
	               Transform transform)
	: _source(std::move(source))
	, _transform(std::move(transform))
	{
		if(!_source.empty())
		{
			_result = _transform(_source.front());
		}
	}

	bool empty() const
	{
		return _source.empty();
	}

	const Result &front() const
	{
		return _result;
	}

	void next()
	{
		_source.next();
		if(!_source.empty())
			_result = _transform(_source.front());
	}

};

// specialization for references
template <class SourceStream, class Transform>
class TransformStream<
	SourceStream,
	Transform,
	typename std::enable_if<
		std::is_reference<RESULT_TYPE>::value
	>::type
>
: public Stream<
	const RESULT_TYPE,
	TransformStream<SourceStream, Transform>
>
{
	typedef typename std::remove_reference<RESULT_TYPE>::type Result;

	SourceStream _source;
	Transform _transform;
	Result *_result;
public:

	TransformStream(SourceStream source,
	               Transform transform)
	: _source(std::move(source))
	, _transform(std::move(transform))
	{
		if(!_source.empty())
		{
			_result = &_transform(_source.front());
		}
	}

	bool empty() const
	{
		return _source.empty();
	}

	const Result &front() const
	{
		return *_result;
	}

	void next()
	{
		_source.next();
		if(!_source.empty())
			_result = &_transform(_source.front());
	}

};


#undef RESULT_TYPE
template <class Transform>
struct TransformPartial
{
	Transform t;
};


template <class SourceStream, class Transform>
TransformStream<SourceStream, Transform> operator |(SourceStream r, TransformPartial<Transform> t)
{
	return TransformStream<SourceStream, Transform>(std::move(r), std::move(t.t));
}

template <class Transform>
TransformPartial<Transform> transformed(Transform t)
{
	return {std::move(t)};
}

template <class SourceStream, class Filter>
class FilterStream: public Stream<
	typename SourceStream::value_type,
	FilterStream<SourceStream, Filter>
>
{
	SourceStream _source;
	Filter _filt;
public:
	FilterStream(SourceStream r, Filter f)
	: _source(std::move(r))
	, _filt(std::move(f))
	{
		while(!_source.empty() && !_filt(_source.front()))
		{
			_source.next();
		}
	}


	bool empty() const
	{
		return _source.empty();
	}

	typename SourceStream::reference_type front() const
	{
		return _source.front();
	}

	void next()
	{
		_source.next();
		while(!_source.empty() && !_filt(_source.front()))
		{
			_source.next();
		}
	}

};

template <class Filter>
struct FilterPartial
{
	Filter f;
};

template <class SourceStream, class Filter>
FilterStream<SourceStream, Filter> operator |(SourceStream r, FilterPartial<Filter> f)
{
	return {std::move(r), std::move(f.f)};
}

template <class Filter>
FilterPartial<Filter> filtered(Filter f)
{
	return {std::move(f)};
}


template <class SourceStream1, class SourceStream2>
class PairStream: public Stream<
	std::pair<
		typename SourceStream1::reference_type,
		typename SourceStream2::reference_type
	>,
	PairStream<SourceStream1, SourceStream2>,
	std::pair<
		typename SourceStream1::reference_type,
		typename SourceStream2::reference_type
	>
>
{
	SourceStream1 _source1;
	SourceStream2 _source2;

	typedef std::pair<
		typename SourceStream1::reference_type,
		typename SourceStream2::reference_type
	> Result;


public:
	PairStream(SourceStream1 s1, SourceStream2 s2)
	: _source1(std::move(s1))
	, _source2(std::move(s2))
	{
	}


	bool empty() const
	{
		return _source1.empty() || _source2.empty();
	}

	Result front() const
	{
		return {_source1.front(), _source2.front()};
	}

	void next()
	{
		_source1.next();
		_source2.next();
	}

};

template <class SourceStream1, class SourceStream2>
PairStream<SourceStream1, SourceStream2> paired(SourceStream1 s1, SourceStream2 s2)
{
	return {std::move(s1), std::move(s2)};
}

template <class Num>
class CounterStream: public Stream<Num, CounterStream<Num>, Num>
{
	Num _n;
public:
	CounterStream(Num first)
	: _n(first)
	{ }

	bool empty() const { return false; }
	Num front() const { return _n; }
	void next()
	{
		++_n;
	}
};

template <class Num=size_t>
CounterStream<Num> count(Num first=0)
{
	return {first};
}

struct EnumeratePartial
{
	size_t start;
};

inline EnumeratePartial enumerated(size_t start=0)
{
	return {start};
}

template <class Stream>
PairStream<CounterStream<size_t>, Stream> operator |(Stream r, EnumeratePartial e)
{
	return {count(e.start), std::move(r)};
}


template <class StreamT>
class TakeFirstStream: public Stream<
	typename StreamT::value_type,
	TakeFirstStream<StreamT>,
	typename StreamT::reference_type
>
{
	size_t _i;
	size_t _n;
	StreamT _source;
public:
	TakeFirstStream(size_t n, StreamT source)
	: _i(0)
	, _n(n)
	, _source(std::move(source))
	{ }

	bool empty() const { return _i >= _n || _source.empty(); }
	typename StreamT::reference_type front() const { return _source.front(); }
	void next() { _source.next(); ++_i; }

};

struct TakePartial
{
	size_t n;
};

inline TakePartial take(size_t n)
{
	return TakePartial{n};
}

template <class Stream>
TakeFirstStream<Stream> operator |(Stream r, TakePartial p)
{
	return {p.n, std::move(r)};
}


template <class StreamT>
class InterposeStream: public Stream<
	typename StreamT::value_type,
	InterposeStream<StreamT>,
	typename StreamT::reference_type
>
{
	StreamT _source;
	typename StreamT::value_type _sep;
	bool _useSource;
public:
	InterposeStream(typename StreamT::value_type sep, StreamT source)
	: _source(std::move(source))
	, _sep(std::move(sep))
	, _useSource(true)
	{ }

	bool empty() const { return _source.empty(); }
	typename StreamT::reference_type front() const { return _useSource? _source.front() : _sep; }
	void next() 
	{
		if(_useSource)
		{
			_source.next();
		}

		_useSource = !_useSource;
	}
};

template <class T>
struct InterposePartial
{
	T sep;
};

template <class T>
InterposePartial<T> interposed(T sep)
{
	return {std::move(sep)};
}

template <class Stream, class T>
InterposeStream<Stream> operator |(Stream r, InterposePartial<T> p)
{
	static_assert(std::is_convertible<T, typename Stream::value_type>::value,
	              "Interposed delimiter must be implicitly convertible to stream's value type.");
	return {std::move(p.sep), std::move(r)};
}

template <class Stream, class It, class=typename std::enable_if<IsStream<Stream>::value>>
void copy(Stream &r, It it)
{
	std::copy(r.begin(), r.end(), it);
}

template <class Stream>
std::vector<typename std::remove_cv<typename Stream::value_type>::type>
toVector(Stream &r)
{
	std::vector<typename std::remove_cv<typename Stream::value_type>::type> result;
	copy(r, std::back_inserter(result));
	return result;
}

template <class D>
struct DynamicCast
{
	template <class S>
	const D *operator()(const std::unique_ptr<S> &p) const
	{
		return dynamic_cast<const D *>(p.get());
	}

	template <class S>
	const D *operator()(const S *p) const
	{
		return dynamic_cast<const D *>(p);
	}

	template <class S>
	std::shared_ptr<D> *operator()(const std::shared_ptr<S> &p) const
	{
		return std::dynamic_pointer_cast<D>(p);
	}
};

template <class D>
struct StaticCast
{
	template <class S>
	D *operator()(S *p) const
	{
		return static_cast<D *>(p);
	}

	template <class S>
	std::shared_ptr<D> *operator()(const std::shared_ptr<S> &p) const
	{
		return std::static_pointer_cast<D>(p);
	}
};

namespace detail 
{
	struct AddrOf
	{
		template <class T>
		T *operator()(T &p) const
		{
			return &p;
		}
	};

	struct Deref
	{
		template <class T>
		T &operator()(T *p) const
		{
			return *p;
		}
	};

	template <class F, class G>
	struct Composition
	{
		F f;
		G g;


		template <class T>
		auto operator()(T &&x) -> decltype(f(g(x)))
		{
			return f(g(std::forward<T>(x)));
		}
	};

	template <class F>
	struct Negation
	{
		F f;

		template <class T>
		bool operator()(T &&x)
		{
			return !f(std::forward<T>(x));
		}
	};


	template <class F>
	struct PairUnpacked
	{
		F f;

		#define EXPR \
			(f(p.first, p.second))

		template <class T, class U>
		auto operator()(const std::pair<T, U> &p) -> decltype(EXPR)
		{
			return EXPR;
		}

		#undef EXPR
	};

	struct Nonzero
	{
		template <class T>
		bool operator()(T x) const
		{
			return x != 0;
		}
	};

	template <class T>
	struct DynamicCastPartial
	{
	};
}

namespace
{
	detail::AddrOf addrOf __attribute__((__unused__));
	detail::Deref deref __attribute__((__unused__));
	detail::Nonzero nonzero __attribute__((__unused__));

}

template <class F, class G>
detail::Composition<F, G> compose(F f, G g)
{
	return {std::move(f), std::move(g)};
}

template <class F>
detail::PairUnpacked<F> pairUnpacked(F f)
{
	return {std::move(f)};
}

template <class F>
TransformPartial<detail::PairUnpacked<F>> pairTransformed(F f)
{
	return transformed(pairUnpacked(std::move(f)));
}

template <class T, class=typename std::enable_if<IsStream<T>::value>::type>
bool any(T strm)
{
	for(bool x : strm) if(x) return true;
	return false;
}

template <class T, class=typename std::enable_if<IsStream<T>::value>::type>
bool all(T strm)
{
	for(bool x : strm) if(!x) return false;
	return true;
}

template <class T>
detail::DynamicCastPartial<T> dynamicCasted()
{
	return {};
}

template <class Stream, class T>
FilterStream<TransformStream<Stream, DynamicCast<T>>, detail::Nonzero> 
operator |(Stream r, detail::DynamicCastPartial<T> p)
{
	return r | transformed(DynamicCast<T>()) | filtered(nonzero);
}



} // namespace streams

#endif // STREAM_HPP_U2MOFE
