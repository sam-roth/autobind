#pragma once


#include <Python.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>


// #define PY_MODULE(name, kwargs...) struct __attribute__((annotate("pymodule:" #name ":" #kwargs))) {}
#define PY_EXPORT __attribute__((annotate("pyexport")))
#define PY_ARGS   __attribute__((annotate("pyargs")))
#define PY_KWARGS __attribute__((annotate("pykwargs")))
#define PY_SYMIDENT __attribute__((annotate("pysymident")))
#define PY_CONVERTER(pytype) __attribute__((annotate("py:convert:" #pytype)))
#define PY_MODULE(name) namespace __attribute__((annotate("py:module:" #name))) {}
#define PY_DOCSTRING(text) namespace __attribute__((annotate("pydocstring:" text))) { }
#define PY_OPERATOR(name) __attribute__((annotate("pyoperator:" #name)))
#define PY_GETTER(name) __attribute__((annotate("pygetter:"#name)))
#define PY_SETTER(name) __attribute__((annotate("pysetter:"#name)))

#ifndef PY_NO_KEYWORDS
	#define pyexport PY_EXPORT
	#define pymodule PY_MODULE
	#define pydocstring PY_DOCSTRING
	#define pyoperator PY_OPERATOR
	#define pygetter PY_GETTER
	#define pysetter PY_SETTER
#endif


class PY_SYMIDENT SymbolIdentitiesPrivate
{
	// magic declarations
	// do not remove
	typedef struct S                 DummyType;
	typedef std::string              StringType;
	typedef std::vector<S>           VectorType;
	typedef std::map<S, S>           MapType;
};

template <class T>
struct PyConversion
{
#ifndef AUTOBIND_RUN
	static_assert(sizeof(T) != sizeof(T),
	              "No specialization of PyConversion<> was found for this type. "
	              "This indicates a problem with your inputs to autobind.");
	// static T load(PyObject *);
	// static PyObject *dump(const T &);
#else
	// prevents compile errors while running autobind about missing specializations that
	// will be automatically filled in later.
	static T &load(PyObject *);
	static PyObject *dump(const T &);
#endif
};

namespace python
{
	namespace protocols
	{
		namespace detail
		{
			struct UnimplTag { };

			template <class U, U>
			class Check
			{

			};

			// based on http://www.gockelhut.com/c++/articles/has_member
			template <class T, class NameGetter>
			class HasMemberImpl
			{
				typedef char matched;
				typedef long unmatched;

				template <class C>
				static matched f(typename NameGetter::template get<C> *);

				template <class C>
				static unmatched f(...);

			public:
				static const bool value = (sizeof(f<T>(0)) == sizeof(matched));
			};

			template <class T, class NameGetter>
			class HasMember: public std::integral_constant<
				bool,
				HasMemberImpl<T, NameGetter>::value
			>
			{ };


			struct CheckHasRepr
			{
				template <class T, std::string(T::*)() const = &T::repr>
				struct get { };
			};

			template <class T>
			class HasRepr: public HasMember<T, CheckHasRepr> { };
			struct CheckHasStr
			{
				template <class T, std::string(T::*)() const = &T::str>
				struct get { };
			};

			template <class T>
			class HasStr: public HasMember<T, CheckHasStr> { };
		}

		template <class T, class Enable=void>
		struct Str: public detail::UnimplTag
		{

		};


		template <class T>
		struct Str<T, typename std::enable_if<detail::HasStr<T>::value>::type>
		{
			static std::string convert(const T &x)
			{
				return x.str();
			}
		};
		
		template <class T, class Enable=void>
		struct Repr: public detail::UnimplTag
		{
		};

		template <class T>
		struct Repr<T, typename std::enable_if<detail::HasRepr<T>::value>::type>
		{
			static std::string convert(const T &x)
			{
				return x.repr();
			}
		};


		template <class T, class Enable=void>
		struct Buffer: public detail::UnimplTag
		{
			static int getBuffer(T &x, Py_buffer *view, int flags);
			static void releaseBuffer(T &x, Py_buffer *view);

		};


		#define ENABLE_IF(x...) typename std::enable_if<x >::type
		#define IS_UNIMPL(x...) std::is_base_of<UnimplTag, x >::value
		namespace detail
		{

			template <class T, class U, class Enable=void>
			struct BufferProcs 
			{
				static int getbuffer(U *exporter,
				                     Py_buffer *view,
				                     int flags)
				{
					view->obj = reinterpret_cast<PyObject*>(exporter);
					int r =  Buffer<T>::getBuffer(exporter->object,
					                              view,
					                              flags);
					Py_XINCREF(view->obj);
					return r;
				}

				static void releasebuffer(U *exporter,
				                          Py_buffer *view)
				{
					Buffer<T>::releaseBuffer(exporter->object,
					                         view);
				}

				static PyBufferProcs *get()
				{
					static PyBufferProcs result = {
						(getbufferproc) &getbuffer,
						(releasebufferproc) &releasebuffer
					};

					return &result;
				}
			};

			template <class T, class U>
			struct BufferProcs<T, U, ENABLE_IF(IS_UNIMPL(Buffer<T>))>
			{
				static PyBufferProcs *get()
				{
					return 0;
				}
			};



			template <class T, class U, class Enable=void>
			struct StrConverter;

			template <class T, class U>
			struct StrConverter<T, U, ENABLE_IF(IS_UNIMPL(Str<T>))>
			{
				static reprfunc get() { return 0; }
			};


			template <class T, class U>
			struct StrConverter<T, U, typename std::enable_if<!std::is_base_of<UnimplTag, Str<T>>::value>::type>
			{
				static PyObject *converter(U *self)
				{
					auto result = Str<T>::convert(self->object);
					return PyConversion<decltype(result)>::dump(result);
				}

				static reprfunc get() { return (reprfunc) &converter; }
			};

			template <class T, class U, class Enable=void>
			struct ReprConverter;

			template <class T, class U>
			struct ReprConverter<T, U, typename std::enable_if<std::is_base_of<UnimplTag, Repr<T>>::value>::type>
			{
				static reprfunc get() { return 0; }
			};


			template <class T, class U>
			struct ReprConverter<T, U, typename std::enable_if<!std::is_base_of<UnimplTag, Repr<T>>::value>::type>
			{
				static PyObject *converter(U *self)
				{
					auto result = Repr<T>::convert(self->object);
					return PyConversion<decltype(result)>::dump(result);
				}

				static reprfunc get() { return (reprfunc) &converter; }
			};
		}

		#undef ENABLE_IF
		#undef IS_UNIMPL

	}

	namespace
	{
		void disposePyObject(PyObject *o)
		{
			Py_XDECREF(o);
		}

		void keepPyObject(PyObject *o) { }

		std::shared_ptr<PyObject> borrow(PyObject *o)
		{
			Py_XINCREF(o);
			return std::shared_ptr<PyObject>(o, disposePyObject);
		}

		std::shared_ptr<PyObject> getNone()
		{
			Py_XINCREF(Py_None);
			return std::shared_ptr<PyObject>(Py_None, disposePyObject);
		}
		
	}
	
	inline bool isinstance(PyObject &obj, PyObject &ty)
	{
		return PyObject_IsInstance(&obj, &ty);
	}



	class Exception: public std::exception
	{
	public:
		const char *what() const throw()
		{
			return "<python exception>";
		}
	};

	class IteratorRef
	{
		std::shared_ptr<PyObject> _iter;
	public:
		IteratorRef(PyObject &obj)
		: _iter(borrow(PyObject_GetIter(&obj)))
		{
			if(!_iter)
			{
				throw Exception();
			}
		}

		operator bool() const
		{
			return _iter != nullptr;
		}

		std::shared_ptr<PyObject> next()
		{
			if(auto result = PyIter_Next(_iter.get()))
			{
				return borrow(result);
			}
			else
			{
				_iter = nullptr;
				return {};
			}
		}
	};

	inline IteratorRef iter(PyObject &obj)
	{
		return IteratorRef(obj);
	}

	class ListRef
	{
		std::shared_ptr<PyObject> _obj;
	public:
		ListRef(std::shared_ptr<PyObject> p)
		: _obj(p) { }


		size_t size() const
		{
			auto res = PySequence_Size(_obj.get());
			if(res < 0)
			{
				throw Exception();
			}

			return res;
		}

		template <class T>
		void set(size_t index, const T &value)
		{
			if(PySequence_SetItem(_obj.get(), index, PyConversion<T>::dump(value)) == -1)
			{
				throw Exception();
			}
		}

		template <class T>
		T get(size_t index) const
		{
			return PyConversion<T>::load(PySequence_GetItem(_obj.get(), index));
		}

		template <class T>
		void append(const T &item)
		{
			if(PyList_Append(_obj.get(), PyConversion<T>::dump(item)))
			{
				throw python::Exception();
			}
		}
	};

	class ObjectRef
	{
		std::shared_ptr<PyObject> _obj;
	public:
		ObjectRef(std::shared_ptr<PyObject> p)
		: _obj(p) { }

		ObjectRef()
		: _obj(getNone()) { } // for STL containers

		static ObjectRef none()
		{
			return ObjectRef(getNone());
		}


		bool operator <(const ObjectRef &other) const
		{
			int rv = PyObject_RichCompareBool(_obj.get(), other._obj.get(), Py_LT);
			if(rv < 0)
			{
				throw python::Exception();
			}
			else
			{
				return rv != 0;
			}
		}

		bool operator ==(const ObjectRef &other)
		{
			int rv = PyObject_RichCompareBool(_obj.get(), other._obj.get(), Py_EQ);
			if(rv < 0)
			{
				throw python::Exception();
			}
			else
			{
				return rv != 0;
			}
		}

		const std::shared_ptr<PyObject> &pyObject() const
		{
			return _obj;
		}
	};

	template <class T>
	class Handle
	{
		ObjectRef _obj; // held to ensure refcount remains > 0
		T &_ref;
	public:

		Handle(const ObjectRef &r)
		: _obj(r)
		, _ref(PyConversion<T>::load(r.pyObject().get()))
		{ }

		T &operator *() const
		{
			return _ref;
		}

		T *operator ->() const
		{
			return &_ref;
		}

		T &data() const
		{
			return _ref;
		}
	};

};

template <class T>
struct PyConversion<python::Handle<T>>
{
	static python::Handle<T> load(PyObject *obj)
	{
		return python::Handle<T>(python::ObjectRef(python::borrow(obj)));
	}
};


template <>
struct PyConversion<python::ListRef>
{
	static python::ListRef load(PyObject *o)
	{
		return python::ListRef(python::borrow(o));
	}
};


template <>
struct PyConversion<python::ObjectRef>
{
	static python::ObjectRef load(PyObject *o)
	{
		return python::ObjectRef(python::borrow(o));
	}


	static PyObject *dump(const python::ObjectRef &oref)
	{
		auto result = oref.pyObject().get();
		Py_XINCREF(result);
		return result;
	}
};


template <>
struct PyConversion<python::IteratorRef>
{
	static python::IteratorRef load(PyObject *o)
	{
		return python::iter(*o);
	}
};




template <>
struct PyConversion<int>
{
	static int load(PyObject *o)
	{
		int res = PyLong_AsLong(o);
		if(PyErr_Occurred())
		{
			throw python::Exception();
		}
		return res;
	}

	static PyObject *dump(int i)
	{
		auto res = PyLong_FromLong(i);
		if(!res)
		{
			throw python::Exception();
		}

		return res;
	}
};

template <class T>
struct PyConversion<std::vector<T> >
{
	static std::vector<T> load(PyObject *obj)
	{
		auto it = python::iter(*obj);

		std::vector<T> result;

		while(auto value = it.next())
		{
			result.push_back(PyConversion<T>::load(value.get()));
		}

		return result;
	}

	static PyObject *dump(const std::vector<T> &v)
	{
		auto lst = PyList_New(0);
		auto lref = PyConversion<python::ListRef>::load(lst);


		for(const auto &item : v)
		{
			lref.append(item);
		}

		return lst;
	}
};

template <>
struct PyConversion<std::string>
{
	static std::string load(PyObject *obj)
	{
		if(auto res = PyUnicode_AsUTF8(obj))
		{
			return res;
		}
		else
		{
			throw python::Exception();
		}
	}

	static PyObject *dump(const std::string &s)
	{
		auto res = PyUnicode_DecodeUTF8(s.c_str(), s.size(), "surrogateescape");
		if(!res)
		{
			throw python::Exception();
		}
		return res;
	}
};

