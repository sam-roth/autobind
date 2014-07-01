
Autobind C++ API
================

.. highlight:: cpp

Annotations
-----------

.. index:: pymodule (C macro)
.. c:macro:: module annotation AB_MODULE(name)

    Specify the Python module name for this translation unit. This must be
    placed before any AB_EXPORT declaration. There may not be more than one
    AB_MODULE declaration in a translation unit.


    :keyword form: ``pymodule``
    :param name: A valid, unquoted Python identifier indicating the module name. 
                 This must be the same as the name of the dynamic library emitted,
                 sans ``.so``, ``.dll``, or ``.dylib``.

    

    Example::
    
        #include <autobind.hpp>
        AB_MODULE(spam);
    
.. index:: pydocstring (C macro)
.. c:macro:: module annotation AB_DOCSTRING(text)

    Specify the docstring for the module.

    :keyword form: ``pydocstring``

    Example::
        
        #include <autobind.hpp>
        AB_MODULE(mongoose);
        AB_DOCSTRING("Python bindings for Boost.MongooseTraits2");



.. index:: pyexport (C macro)
.. c:macro:: declaration annotation AB_EXPORT
        
    Make a class or function available from Python. 
    
    Doxygen-style comments near the declaration will be provided to
    Python as docstrings.

    :keyword form: ``pyexport``

    For functions, place the annotation before the type name::
        
        AB_EXPORT void hello()
        {
            std::cout << "Hello, world\n";
        }

    You may annotate an existing function by redeclaring it, so long as you are
    careful to redeclare it in exactly the same way::

        AB_EXPORT int system(const char *);

    Annotate classes and structs by placing the annotation between the initial
    keyword and the name of the aggregate::
        
        struct AB_EXPORT Spam { ... };
        class  AB_EXPORT Spam { ... };

.. index:: pygetter (C macro), pysetter (C macro)
.. c:macro:: member function annotation AB_GETTER(name)
             member function annotation AB_SETTER(name)

    Make a getter and optionally, a setter, available from Python.

    :keyword form: ``pygetter``, ``pysetter``

    These annotations are only valid for member functions::

        class AB_EXPORT Sandwich
        {
            static constexpr bool _has_spam = true;
        public:
            void AB_SETTER(has_spam) set_has_spam(bool value)
            {
                if(!value)
                    throw std::logic_error("spam");
            }

            bool AB_GETTER(has_spam) has_spam() const
            {
                return _has_spam;
            }
        };



    

Client-Specializable Templates
------------------------------

.. cpp:class:: python::Conversion<T>
    
    Specialize this class template to teach autobind to perform Python type conversions.

    .. cpp:function:: static T load(PyObject *object)

    .. note::   You may return a reference from this function, so long as the referenced
                object will remain valid at least until the PyObject ``object`` is deallocated.

    .. cpp:function:: static PyObject *dump(const T &)


    Here's an example, taken from ``autobind.hpp``::

        template <>
        struct python::Conversion<int>
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

.. s*

.. cpp:class:: python::protocols::Str<T>
               python::protocols::Repr<T>

    Specialize these class templates to implement the :py:meth:`~object.__str__` and
    :py:meth:`~object.__repr__` protocols, respectively.

    There is also an additional second template parameter, ``Enable``, (void
    by default), which you may use with :cpp:class:`std::enable_if\<>`. 


    .. cpp:function:: static std::string convert(const T &)

    .. hint:: These templates come pre-specialized for types that define 
              a const, `std::string`\ -returning, method ``str`` or ``repr``.


    Example::

        template <>
        struct python::protocols::Repr<Duck>
        {
            static std::string convert(const Duck &d)
            {
                return "Duck(quacks=" + std::to_string(d.quacks()) + ")";
            }
        };


Python Objects
--------------


.. cpp:class:: python::ObjectRef

 An automatically reference-counted reference to a Python :py:obj:`object`.

 .. cpp:function:: ObjectRef(std::shared_ptr<PyObject> p)

     Construct an :cpp:class:`ObjectRef` from a shared_ptr<:c:type:`PyObject`>. 

     Please consider using PyConverter<ObjectRef>::load(obj) to create ObjectRefs,
     rather than this constructor.

     .. danger::
         If you must construct your own shared_ptr<>, don't use the default
         shared_ptr<> deleter. Instead use a function that will call
         Py_XDECREF.

 .. cpp:function:: ObjectRef()

     Construct an :cpp:class:`ObjectRef` pointing at :py:obj:`None`.

 .. cpp:function:: bool operator <(const ObjectRef &other) const
     
     Equivalent to the Python expression ``this < other``.

     Implemented using :c:func:`PyObject_RichCompareBool`.

 .. cpp:function:: bool operator ==(const ObjectRef &other) const

     Equivalent to the Python expression ``this == other``.

     Implemented using :c:func:`PyObject_RichCompareBool`.

 .. cpp:function:: const std::shared_ptr<PyObject> &pyObject() const

     Get the shared_ptr<PyObject> that backs this :cpp:class:`ObjectRef`.

.. cpp:class:: python::Exception: public std::exception
    
    This exception is thrown when an Autobind API catches a Python exception.
    If it reaches the Python API boundary, it is caught and
    :c:func:`PyErr_Clear` is not called, allowing the Python exception to
    propagate into Python user code. If you catch it, the exception will not
    pass into Python code, since :c:func:`PyErr_Clear` is called before
    returning control to Python unless the binding code catches an exception.


    .. cpp:function:: const char *what() const noexcept

        Always returns ``"<python exception>"`` for now. I plan to
        wire this up to show the :py:func:`repr` of the Python exception.

.. s*

.. cpp:class:: python::Handle<T>

    :cpp:class:`Handle\<T>` is a non-nullable smart pointer for C++ types stored within
    a PyObject. It uses a combination of :cpp:class:`std::shared_ptr\<>` and Python reference
    counting to ensure that the object is only disposed once all references have gone out of
    scope.
    
    :static assertions: 
        * ``!std::is_reference<T>::value``
        * ``std::is_reference<typename ConversionLoadResult<T>::type>::value``
        

    .. cpp:function:: Handle(const ObjectRef &r)

        Construct this :cpp:class:`Handle\<T>` by using a reference-returning specialization
        of :cpp:class:`python::Conversion\<T>`. 

        :throws: An exception if the type conversion fails.

    .. cpp:function::   T &operator *() const
                        T *operator ->() const
                        T &data() const
                        T &get() const

        Return a reference or pointer to the held value.
    


Generic Utilities
-----------------


.. cpp:class:: autobind::Optional<T>
    
    A nullable value type.

    This class serves the same purpose as :cpp:class:`boost::optional\<>` without making
    the generated code dependent on Boost.
    
    In case you're wondering, yes, in fact, it does use :cpp:class:`std::aligned_storage\<>`.


    .. cpp:function::   Optional<T>(const T &)
                        Optional<T>(T &&)

        Initialize the object with the value given.


    .. cpp:function::   Optional<T>()
        
        Initialize the object, marking it as invalid.

    .. cpp:function::   Optional<T>(const Optional<T> &)
                        Optional<T>(Optional<T> &&)

        Copy or move another :cpp:class:`Optional\<T>`. If valid, the copy or move 
        constructor of the held value will also be called.

    .. cpp:function::   Optional<T> &operator =(const Optional<T> &)
                        Optional<T> &operator =(Optional<T> &&)

        Assign the :cpp:class:`Optional\<T>` from another :cpp:class:`Optional\<T>`. If valid, the
        copy or move operator from the held value will also be called.

    .. cpp:function::   void reset(const Optional<T> &)
                        void reset(Optional<T> &&)

        Assign the :cpp:class:`Optional\<T>` from another :cpp:class:`Optional\<T>`. This differs
        from :cpp:func:`~::operator=` in that it uses the copy or move *constructor*,
        rather than the objects ``operator=``.

    .. cpp:function::   operator bool() const
                        bool exists() const
        
        Test whether the :cpp:class:`Optional\<T>` is valid.

    .. cpp:function::   T &operator *() 
                        T *operator ->() 
                        T &get() 
                        const T &operator *()  const
                        const T *operator ->()  const
                        const T &get()  const

        Retrieve the value of the `Optional`.        

        .. Warning::
            
            If the `Optional` is invalid, the result of any of these functions
            is undefined. Using it informs your compiler that *you* think that
            the :cpp:class:`Optional\<T>` will always be valid along that code path. 

    .. cpp:function::   T &orElse(T &)
                        const T &orElse(const T &) const
        
        Retrieves the value of the :cpp:class:`Optional\<T>` if it is valid, otherwise returns
        the passed in value.



.. index:: autobind::makeOptional<T, Args...> (C++ function)
.. cpp:function:: autobind::Optional<T> autobind::makeOptional<T, Args...>(Args &&... args)

        Create a new :cpp:class:`Optional\<T>`, initialized in place by forwarding the
        arguments provided to T's constructor.

        
        .. tip::
            
            Your C++ compiler will automatically infer the ``Args...`` template parameter.
            You should buy it a beer.



