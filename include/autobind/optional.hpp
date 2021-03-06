///
/// @file optional.hpp
///
/// A reimplementation of boost::optional.
/// The Boost implementation was not used to avoid adding dependencies to
/// generated code.

#ifndef OPTIONAL_HPP_30062Y
#define OPTIONAL_HPP_30062Y
#include <utility>
#include <ostream>
#include <type_traits>
#define ENABLE_IF(x...) typename std::enable_if<(x)>::type
namespace autobind {
namespace detail
{
	template <class T>
	class Storage
	{
		typename std::aligned_storage<sizeof(T), alignof(T)>::type data;

	public:

		template <class... Args>
		void construct(Args &&... args)
		{
			new (static_cast<void*>(&data)) T(std::forward<Args>(args)...);
		}

		T &get()
		{
			return *reinterpret_cast<T *>(&data);
		}

		const T &get() const
		{
			return *reinterpret_cast<const T *>(&data);
		}

		void destruct()
		{
			get().~T();
		}
	};

	template <class T>
	class Storage<T &>
	{
		T *data;
	public:
		void construct(T &ref)
		{
			data = &ref;
		}

		T &get() const
		{
			return *data;
		}

		void destruct()
		{

		}
	};

	struct EmplaceTag{};
	struct DummyTag{};
}

template <class T>
class Optional
{
	bool _exists;
	detail::Storage<T> _storage;
public:
	Optional(const Optional &other)
	: _exists(false)
	{
		reset(other);
	}

	Optional(Optional &&other)
	: _exists(false)
	{
		reset(std::move(other));
	}

	Optional()
	: _exists(false)
	{
	}

	Optional(const T &value)
	: _exists(false)
	{
		_storage.construct(value);
		_exists = true;
	}

	template <
		class U,
		class=ENABLE_IF(std::is_same<T, U>::value 
		                && !std::is_reference<T>::value)
	>
	Optional(T &&value)
	: _exists(false)
	{
		_storage.construct(std::move(value));
		_exists = true;
	}

	template <class... Args>
	Optional(detail::EmplaceTag, Args &&... args)
	: _exists(false)
	{
		_storage.construct(std::forward<Args>(args)...);
		_exists = true;
	}

	~Optional()
	{
		if(_exists) _storage.destruct();
		_exists = false;
	}


	void reset(const Optional &value)
	{
		if(this == &value) return;

		if(_exists)
		{
			_storage.destruct();
			_exists = false;
		}

		if(value._exists)
		{
			_storage.construct(*value);
			_exists = true;
		}
	}


	template <class U, class=ENABLE_IF(!std::is_reference<T>::value 
	                                   && std::is_same<Optional<T>, U>::value)>
	void reset(U &&value)
	{
		if(this == &value) return;

		if(_exists)
		{
			_storage.destruct();
			_exists = false;
		}

		if(value._exists)
		{
			_storage.construct(std::move(*value));
			value._exists = false;
			_exists = true;
		}
	}



	Optional &operator =(const Optional &other)
	{
		if(&other == this) return *this;

		if(_exists && other._exists)
		{
			_storage.get() = other._storage.get();
		}
		else if(_exists)
		{
			_storage.destruct();
			_exists = false;
		}
		else if(other._exists)
		{
			_storage.construct(other._storage.get());
			_exists = true;
		}

		return *this;
	}

	Optional &operator =(Optional &&other)
	{
		if(&other == this) return *this;

		if(_exists && other._exists)
		{
			_storage.get() = std::move(other._storage.get());
			other._storage.destruct();
			other._exists = false;
		}
		else if(_exists)
		{
			_storage.destruct();
			_exists = false;
		}
		else if(other._exists)
		{
			_storage.construct(std::move(other._storage.get()));
			_exists = true;
			other._storage.destruct();
			other._exists = false;
		}

		return *this;
	}

	operator bool() const
	{
		return _exists;
	}

	bool exists() const
	{
		return _exists;
	}

	const T &get() const
	{
		assert(_exists);
		return _storage.get();
	}

	T &get()
	{
		assert(_exists);
		return _storage.get();
	}

	const typename std::remove_reference<T>::type 
	*operator ->() const
	{
		return &get();
	}

	typename std::remove_reference<T>::type 
	*operator ->()
	{
		return &get();
	}

	T &operator *()
	{
		return get();
	}

	const T &operator *() const
	{
		return get();
	}


	const T &orElse(const T &value) const
	{
		if(*this)
		{
			return **this;
		}
		else
		{
			return value;
		}
	}

	T &orElse(T &value)
	{
		if(*this)
		{
			return **this;
		}
		else
		{
			return value;
		}
	}

	friend std::ostream &operator <<(std::ostream &out, const Optional &opt)
	{
		if(opt)
		{
			out << "Optional(" << *opt << ")";
		}
		else
		{
			out << "Optional()";
		}

		return out;
	}
};


template <class U, class... Args>
Optional<U> makeOptional(Args &&... args)
{
	return Optional<U>(detail::EmplaceTag{}, std::forward<Args>(args)...);
}


} // autobind

#undef ENABLE_IF
#endif // OPTIONAL_HPP_30062Y


