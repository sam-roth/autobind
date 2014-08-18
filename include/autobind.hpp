// Copyright (c) 2014, Samuel A. Roth. All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can
// be found in the COPYING file.

#ifndef AUTOBIND_HPP_X4U6E9
#define AUTOBIND_HPP_X4U6E9

#include <Python.h>

#include <cxxabi.h>
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <typeinfo>

#include "autobind/optional.hpp"

#define AB_PRIVATE_ANNOTATE(a...)        __attribute__((annotate(a)))
#define AB_PRIVATE_TU_ANNOTATE(a...)     namespace AB_PRIVATE_ANNOTATE(a) { }

#define AB_EXPORT                        AB_PRIVATE_ANNOTATE("pyexport")
#define AB_MODULE(name)                  AB_PRIVATE_TU_ANNOTATE("py:module:" #name)
#define AB_DOCSTRING(text)               AB_PRIVATE_TU_ANNOTATE("pydocstring:" text)
#define AB_GETTER(name)                  AB_PRIVATE_ANNOTATE("pygetter:" #name)
#define AB_SETTER(name)                  AB_PRIVATE_ANNOTATE("pysetter:" #name)
#define AB_NOEXPORT                      AB_PRIVATE_ANNOTATE("pynoexport")

#ifndef AB_NO_KEYWORDS
	#define pyexport    AB_EXPORT
	#define pymodule    AB_MODULE
	#define pydocstring AB_DOCSTRING
	#define pygetter    AB_GETTER
	#define pysetter    AB_SETTER
#endif


namespace python
{

	using autobind::Optional;

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
		static T &load(PyObject *);
		static PyObject *dump(const T &);
		static const char *pythonTypeName();
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

	class ObjectRef;

	class Exception: public std::exception
	{
		mutable std::string _msg;
	public:
		const char *what() const throw();
		ObjectRef asObject() const;
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

	class ListRef;

	class ObjectRef
	{
		std::shared_ptr<PyObject> _obj;
	public:
		ObjectRef(std::shared_ptr<PyObject> p)
		: _obj(p) { }

		ObjectRef()
		: _obj(getNone()) { } // for STL containers

		template <class T>
		static ObjectRef create(const T &value)
		{
			return steal(Conversion<T>::dump(value));
		}

		static ObjectRef steal(PyObject *o)
		{
			return ObjectRef(std::shared_ptr<PyObject>(o, &disposePyObject));
		}

		static ObjectRef borrow(PyObject *o)
		{
			return ObjectRef(::python::borrow(o));
		}

		static ObjectRef none()
		{
			return ObjectRef(getNone());
		}

		PyObject *release()
		{
			// We can't actually release the underlying smart pointer, so just
			// bump the Python refcount.
			auto result = _obj.get();
			Py_XINCREF(result);
			_obj = getNone();
			return result;
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

		bool operator ==(const ObjectRef &other) const
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

		operator PyObject *() const
		{
			return _obj.get();
		}

		template <class T>
		typename ConversionLoadResult<T>::type convert() const
		{
			return Conversion<T>::load(*this);
		}

		template <class T>
		Optional<typename ConversionLoadResult<T>::type> tryConverting() const
		{
			return this->convert<Optional<T>>();
		}

		std::string repr() const;
		std::string str() const;

		ListRef asList() const;


		ObjectRef getattr(const std::string &name) const
		{
			return getattr(name.c_str());
		}
		

		ObjectRef getattr(const char *name) const
		{
			if(auto result = PyObject_GetAttrString(*this, name))
				return steal(result);
			else
				throw Exception();
		}

		void setattr(const std::string &name, ObjectRef o)
		{
			setattr(name.c_str(), o);
		}

		void setattr(const char *name, ObjectRef o)
		{
			if(PyObject_SetAttrString(*this, name, o) == -1)
				throw Exception();
		}


		void delattr(const std::string &name)
		{
			delattr(name.c_str());
		}


		void delattr(const char *name)
		{
			if(PyObject_DelAttrString(*this, name) == -1)
				throw Exception();
		}

		template <class... Args>
		ObjectRef operator ()(const Args &... args)
		{

			if(auto result = PyObject_CallFunctionObjArgs(*this, 
			                                              static_cast<PyObject *>(create(args))...,
			                                              0))
			{
				return steal(result);
			}
			else
			{
				throw python::Exception();
			}
		}

		operator bool() const
		{
			int result = PyObject_IsTrue(*this);
			if(result == -1) throw Exception();
			return result == 1;
		}

		bool operator !() const
		{
			int result = PyObject_Not(*this);
			if(result == -1) throw Exception();
			return result == 1;
		}

		Py_hash_t hash() const
		{
			Py_hash_t result = PyObject_Hash(*this);
			if(result == -1) throw Exception();
			return result;
		}

		bool isinstance(const ObjectRef &ty) const
		{
			int result = PyObject_IsInstance(*this, ty);
			if(result == -1) throw Exception();
			return result == 1;
		}

		bool issubclass(const ObjectRef &base) const
		{
			int result = PyObject_IsSubclass(*this, base);
			if(result == -1) throw Exception();
			return result == 1;
		}

		ObjectRef type() const
		{
			if(auto result = PyObject_Type(*this))
				return steal(result);
			else
				throw Exception();
		}
	};

	#define MBR(ty, name) inline ty name(const ObjectRef &o) { return o.name(); }

	MBR(ObjectRef, type)
	MBR(Py_hash_t, hash)
	MBR(std::string, str)
	MBR(std::string, repr)

	#undef MBR

	inline bool isinstance(const ObjectRef &o, const ObjectRef &ty)
	{
		return o.isinstance(ty);
	}
	
	inline bool issubclass(const ObjectRef &derived, const ObjectRef &base)
	{
		return derived.issubclass(base);
	}

	inline ObjectRef getattr(const ObjectRef &o, const char *name)
	{
		return o.getattr(name);
	}

	inline void setattr(ObjectRef &o, const char *name, const ObjectRef &value)
	{
		o.setattr(name, value);
	}

	inline ObjectRef getattr(ObjectRef &o, const char *name, const ObjectRef &defaultValue)
	{
		try
		{
			return o.getattr(name);
		}
		catch(python::Exception &exc)
		{
			if(PyErr_ExceptionMatches(PyExc_AttributeError))
			{
				return defaultValue;
			}
			else
			{
				throw;
			}
		}
	}


	inline python::ObjectRef Exception::asObject() const
	{
		PyObject *ty=0, *val=0, *tb=0;
		PyErr_Fetch(&ty, &val, &tb);
		PyErr_NormalizeException(&ty, &val, &tb);
		auto result = ObjectRef::borrow(val);
		PyErr_Restore(ty, val, tb);
		return result;
	}
	inline const char *Exception::what() const throw()
	{
		if(_msg.empty())
		{
			try
			{
				auto obj = asObject();
				_msg = obj.str();
			}
			catch(std::exception &exc)
			{
				_msg = "<Python exception>";
			}
		}

		return _msg.c_str();
	}


	class ListRef: public ObjectRef
	{
	public:
		using ObjectRef::ObjectRef;

		ListRef(const ObjectRef &other)
		: ObjectRef(other) { }

		size_t size() const
		{
			auto res = PySequence_Size(*this);
			if(res < 0)
			{
				throw Exception();
			}

			return res;
		}

		template <class T>
		void set(size_t index, const T &value)
		{
			if(PySequence_SetItem(*this, index, Conversion<T>::dump(value)) == -1)
			{
				throw Exception();
			}
		}

		template <class T>
		T get(size_t index) const
		{
			return Conversion<T>::load(PySequence_GetItem(*this, index));
		}

		template <class T>
		void append(const T &item)
		{
			if(PyList_Append(*this, Conversion<T>::dump(item)))
			{
				throw python::Exception();
			}
		}
	};

	inline ListRef ObjectRef::asList() const
	{
		return ListRef(*this);
	}

	template <class T>
	class Handle
	{
		static_assert(!std::is_reference<T>::value,
		              "Handle<T>: T should not be a reference. Handles always hold references.");

		static_assert(std::is_reference<typename ConversionLoadResult<T>::type>::value,
		              "Handle<T> may only be used with types for which python::Conversion<T> returns "
		              "a reference.");

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

		T &get() const
		{
			return _ref;
		}

		template <class U, class=typename std::enable_if<std::is_same<U, bool>::value>::type>
		operator U() const
		{
			static_assert(!std::is_same<U, U>::value,
			              "Handles are non-nullable. Delete this null check.");
		}

		const ObjectRef &asObject() const
		{
			return _obj;
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
	struct Conversion<char>
	{
		static char load(PyObject *o)
		{
			Py_ssize_t n;
			if(auto text = PyUnicode_AsUTF8AndSize(o, &n))
			{
				if(n != 1)
				{
					throw std::invalid_argument("String must consist of a single byte.");
				}

				return *text;
			}
			else
			{
				throw python::Exception();
			}
		}
		
		static PyObject *dump(char c)
		{
			auto res = PyUnicode_DecodeUTF8(&c, 1, "surrogateescape");
			if(!res)
			{
				throw python::Exception();
			}

			return res;
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



	template <class T, class Enable>
	struct Conversion<Optional<T>, Enable>
	{
	private:
		typedef typename std::remove_reference<T>::type U;
	public:
		static Optional<typename ConversionLoadResult<U>::type>
		load(PyObject *obj)
		{
			try
			{
				return Conversion<U>::load(obj);
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

		template <class U>
		static PyObject *dump(const Optional<U> &obj)
		{
			static_assert(std::is_convertible<U, T>::value,
			              "Converter not compatible with this type");

			if(!obj)
			{
				Py_RETURN_NONE;
			}
			else
			{
				return Conversion<T>::dump(*obj);
			}
		}
	};

	inline std::string ObjectRef::repr() const
	{
		return steal(PyObject_Repr(*this)).convert<std::string>();
	}


	inline std::string ObjectRef::str() const
	{
		return steal(PyObject_Str(*this)).convert<std::string>();
	}
	

	template <class T>
	Optional<typename ConversionLoadResult<T>::type> 
	tryConverting(PyObject *obj)
	{
		return Conversion<Optional<typename ConversionLoadResult<T>::type>>::load(obj);
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
#endif // AUTOBIND_HPP_X4U6E9
