#pragma once


#include <Python.h>

#include <cxxabi.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <typeinfo>

#include "optional.hpp"

#define AB_PRIVATE_ANNOTATE(a...)        __attribute__((annotate(a)))
#define AB_PRIVATE_TU_ANNOTATE(a...)     namespace AB_PRIVATE_ANNOTATE(a) { }

#define AB_EXPORT                        AB_PRIVATE_ANNOTATE("pyexport")
#define AB_MODULE(name)                  AB_PRIVATE_TU_ANNOTATE("py:module:" #name)
#define AB_DOCSTRING(text)               AB_PRIVATE_TU_ANNOTATE("pydocstring:" text)
#define AB_GETTER(name)                  AB_PRIVATE_ANNOTATE("pygetter:" #name)
#define AB_SETTER(name)                  AB_PRIVATE_ANNOTATE("pysetter:" #name)

#ifndef AB_NO_KEYWORDS
	#define pyexport    AB_EXPORT
	#define pymodule    AB_MODULE
	#define pydocstring AB_DOCSTRING
	#define pygetter    AB_GETTER
	#define pysetter    AB_SETTER
#endif


namespace python
{
	template <class T, class Enable=void>
	struct Conversion
	{
	#ifndef AUTOBIND_RUN
		static_assert(sizeof(T) != sizeof(T),
		              "No specialization of Conversion<> was found for this type. "
		              "This indicates a problem with your inputs to autobind.");
		// static T load(PyObject *);
		// static PyObject *dump(const T &);
 		// static const char *pythonTypeName();
	#else
		// prevents compile errors while running autobind about missing specializations that
		// will be automatically filled in later.
		static T load(PyObject *)
		{
			throw std::runtime_error("No conversion");
		}
		static PyObject *dump(const T &)
		{
			throw std::runtime_error("No conversion");
		}
// 		static const char *pythonTypeName();
	#endif
	};

	template <class T>
	struct ConversionLoadResult
	{
		typedef decltype(Conversion<T>::load(std::declval<PyObject *>())) type;
	};



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




			struct CheckHasPythonTypeName
			{
				template <class T, const char *(*)() = &T::pythonTypeName>
				struct get { };
			};

			template <class T>
			class HasPythonTypeName: public HasMember<T, CheckHasPythonTypeName> { };

			template <class T, class Enable=void>
			class PythonTypeName
			{
				static const char *get()
				{
					return typeid(T).name();
				}
			};
// 
// 			template <class T>
// 			class PythonTypeName<T,
		}
//
// 		template <class T>
// 		const char *pythonTypeName()
// 		{
// 
// 		}
// 
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
					return Conversion<decltype(result)>::dump(result);
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
					return Conversion<decltype(result)>::dump(result);
				}

				static reprfunc get() { return (reprfunc) &converter; }
			};
		}

		#undef ENABLE_IF
		#undef IS_UNIMPL

	}

	inline void disposePyObject(PyObject *o)
	{
		Py_XDECREF(o);
	}

	inline void keepPyObject(PyObject *o) { }

	inline std::shared_ptr<PyObject> borrow(PyObject *o)
	{
		Py_XINCREF(o);
		return std::shared_ptr<PyObject>(o, disposePyObject);
	}
	
	inline std::shared_ptr<PyObject> getNone()
	{
		Py_XINCREF(Py_None);
		return std::shared_ptr<PyObject>(Py_None, disposePyObject);
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
			if(PySequence_SetItem(_obj.get(), index, Conversion<T>::dump(value)) == -1)
			{
				throw Exception();
			}
		}

		template <class T>
		T get(size_t index) const
		{
			return Conversion<T>::load(PySequence_GetItem(_obj.get(), index));
		}

		template <class T>
		void append(const T &item)
		{
			if(PyList_Append(_obj.get(), Conversion<T>::dump(item)))
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
		, _ref(Conversion<T>::load(r.pyObject().get()))
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

	template <class T>
	struct Conversion<python::Handle<T>>
	{
		static python::Handle<T> load(PyObject *obj)
		{
			return python::Handle<T>(python::ObjectRef(python::borrow(obj)));
		}
	};


	template <>
	struct Conversion<python::ListRef>
	{
		static python::ListRef load(PyObject *o)
		{
			return python::ListRef(python::borrow(o));
		}
	};


	template <>
	struct Conversion<python::ObjectRef>
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
	struct Conversion<python::IteratorRef>
	{
		static python::IteratorRef load(PyObject *o)
		{
			return python::iter(*o);
		}
	};




	template <>
	struct Conversion<int>
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
	struct Conversion<std::vector<T> >
	{
		static std::vector<T> load(PyObject *obj)
		{
			auto it = python::iter(*obj);

			std::vector<T> result;

			while(auto value = it.next())
			{
				result.push_back(Conversion<T>::load(value.get()));
			}

			return result;
		}

		static PyObject *dump(const std::vector<T> &v)
		{
			auto lst = PyList_New(0);
			auto lref = Conversion<python::ListRef>::load(lst);


			for(const auto &item : v)
			{
				lref.append(item);
			}

			return lst;
		}
	};

	template <>
	struct Conversion<std::string>
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


	template <class T>
	autobind::Optional<typename ConversionLoadResult<T>::type> 
	tryConverting(PyObject *obj)
	{
		try
		{
			return Conversion<T>::load(obj);
		}
		catch(Exception &)
		{
			PyErr_Clear();
		}
		catch(std::runtime_error &)
		{
		}

		return {};
	}

	namespace detail
	{

		inline std::string demangle(const char *name)
		{
			int ok;

			char *buff = abi::__cxa_demangle(name, nullptr, nullptr, &ok);
			if(ok == 0)
			{
				std::string result = buff;
				std::free(buff);
				return result;
			}
			else
			{
				return name;
			}
		}

		inline std::string elideTemplateArgs(const std::string &str)
		{
			std::string result;

			int openBracketCount = 0;
			for(char c : str)
			{
				if(c == '>')
				{
					--openBracketCount;
				}

				if(!openBracketCount)
				{
					result.push_back(c);
				}

				if(c == '<')
				{
					openBracketCount++;
				}
			}

			return result;
		}

		inline std::string elidedDemangle(const char *name)
		{
			return elideTemplateArgs(demangle(name));
		}

		template <class T>
		struct ConversionFunc
		{
			struct Value
			{
				const char *errorMessage;
				autobind::Optional<typename ConversionLoadResult<T>::type> value;
			};

			static int convert(PyObject *pyObject, void *address)
			{
				auto &result = *static_cast<typename ConversionFunc<T>::Value *>(address);

				result.value.reset(tryConverting<T>(pyObject));

				if(!result.value)
				{
					PyErr_Format(PyExc_TypeError, "%s",
					             result.errorMessage);

					return 0;
				}
				else
				{
					return 1;
				}
			}


		};
	}

}
