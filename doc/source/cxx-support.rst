
Supported C++ Features
======================

.. highlight:: cpp

Function Declarations
---------------------

Autobind groks normal C++ function declarations: all you need to do is add the
:c:macro:`AB_EXPORT` annotation::

    AB_EXPORT std::vector<int> doubled(const std::vector<int> &numbers);

You may also define the function inline::

    AB_EXPORT std::vector<int> doubled(const std::vector<int> &numbers)
    {
        std::vector<int> result;
        for(int n : numbers) result.push_back(n * 2);
        return result;
    }

Documentation comments attached to the declaration are translated to docstrings::

    /// Double a list of numbers, the hard way.
    AB_EXPORT std::vector<int> doubled(const std::vector<int> &numbers);

    /** Double a list of numbers, the hard way. */
    AB_EXPORT std::vector<int> doubled(const std::vector<int> &numbers);
    
.. code-block:: python

    >>> mymodule.doubled?
    Type:        builtin_function_or_method
    String form: <built-in function doubled>
    Docstring:
    (numbers: std::vector<int>) -> std::vector<int>

    Double a list of numbers, the hard way.

Function overloading is also permitted. Operator overloading is not yet
implemented.

Non-Polymorphic Classes
-----------------------

Autobind can also create bindings for simple C++ classes (and structs, not that
they're any different); however, currently, runtime polymorphism is not
implemented in a useful way. Python classes may shadow, but not override, C++
virtual methods, and Autobind cannot automatically cast derived pointers to
base pointers when generating bindings. Autobind also cannot yet automatically
create bindings for data members of a class. You may, however, provide properties.

To export a class or struct, interpose the annotation ``AB_EXPORT`` between 
the tag (i.e., ``struct`` or ``class``) and the aggregate's identifier::

    class AB_EXPORT Window
    {
        ...
    public:
        Window();
        virtual ~Window();

        void show();

        bool AB_GETTER(visible) isVisible() const;
        void AB_SETTER(visible) setVisible(bool visibility);
    };

If this class were written in Python, we would have written:

.. code-block:: python

    class Window:
        def __init__(self): ...

        def show(self): ...

        @property
        def visible(self): ...

        @visible.setter
        def visible(self, value): ...


All non-template public member functions of an exported class are exported, with the
exception of the destructor, which is called automatically. Uninstantiated
member templates will never be supported generically, as doing so would require
dynamic invocation of a C++ compiler.

Exception Message Marshalling
-----------------------------

Exceptions deriving from :cpp:class:`std::exception` are automatically converted
to Python :py:exc:`RuntimeError`\ s and reraised. The message for the error is taken
from the exception's :cpp:func:`~std::exception::what` method.

  

