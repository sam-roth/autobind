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

#ifndef PY_NO_KEYWORDS
	#define pyexport PY_EXPORT
	#define pymodule PY_MODULE
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
	// static T load(PyObject *);
	// static PyObject *dump(const T &);
};


namespace python
{

	namespace
	{
		void disposePyObject(PyObject *o)
		{
			Py_XDECREF(o);
		}

		std::shared_ptr<PyObject> borrow(PyObject *o)
		{
			Py_XINCREF(o);
			return std::shared_ptr<PyObject>(o, disposePyObject);
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

