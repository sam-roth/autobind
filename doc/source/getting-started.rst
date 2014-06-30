
Getting Started
===============

The bad news: Writing bindings for a native-code module is boring, mechanical,
and error-prone.

The good news: Writing bindings for a native-code module is mechanical. Why not
let your computer do the work for you?

Building Autobind
-----------------
:you'll need:
    • CMake
    • Clang ≥ 3.3 (with headers)
    • Boost (headers and Boost.Regex)
    • Python ≥ 3.3 (for running the wrapper script)

Once you've installed the dependencies, just do::

    $ cmake .
    $ make -j4

If it doesn't work, you found a bug, either in the program or the
documentation. Please file a bug report [#]_.

Using the Wrapper Script
------------------------

The Autobind driver uses Clang LibTooling. The command line parser from
LibTooling gives you a great deal of flexibility and control in running
Clang-based tooling; however, it is also complicated to use. For instance, you
must manually give paths to system header files.

To work around this issue, Autobind has a simpler wrapper script in
``scripts/autobind.py``. This is a Python 3 script, so ensure you have a copy
of Python 3 in order to use it. 

The rest of this document assumes that you're using the wrapper script, and not
the driver directly.

.. index:: autobind.py

Autobind Wrapper Verbs
^^^^^^^^^^^^^^^^^^^^^^

The wrapper script has two modes called verbs, ``generate`` and ``build``.

.. program:: autobind.py generate
.. describe:: $ autobind.py generate -c input-file [-o output-file]

    In the ``generate`` mode, the script reads an input C++ file and generates
    an output C++ file containing the bindings. It does not attempt to compile
    them.

    .. option:: -c input-file
        
        The input C++ file, containing the definitions to be exported. 

    .. option:: -o output-file

        The output C++ file, to which the bindings will be written.

.. program:: autobind.py build

.. describe:: $ autobind.py build -c input-file-1 [-c input-file-2...]

    In the ``build`` mode, the script also reads an input C++ file, but instead
    of writing the bindings to ``STDOUT`` or a user-specified file, it writes
    them to a temporary file, which is then automatically compiled and linked.
    The temporary file is deleted afterwards. The output will be assigned a
    name that is appropriate for your system and placed in the same directory
    as the source file. 

    This mode is provided for convenience, as well as providing a reference for
    how to correctly use autobind with your compiler.


Hello, world!
-------------

.. highlight:: cpp

Open up a new C++ file (hello.cpp) and put down the following [#macro-hygiene]_ ::

    #include <iostream>
    #include <autobind.hpp>
    
    pymodule(hello);

    pyexport void hello_world()
    {
        std::cout << "Hello, world!\n";
    }

The ``pymodule()`` directive instructs Autobind that the module's name is ``hello``. 

.. warning::
    The name of the module, as declared with ``pymodule()`` must match the name
    of the dynamic library, sans extension, exactly. Using the wrong module
    name will make it impossible to load your module. This is true of all
    Python/C module and not just those produced by Autobind.
    

Next comes the, uh well, *easy* part. Open up a terminal and type::
    
    $ /path/to/autobind/scripts/autobind.py build -c hello.cpp

.. highlight:: python

Then, open up a Python3 shell and type::
    
    >>> import hello
    >>> hello.hello_world()
    Hello, world!



.. rubric:: Footnotes

.. [#]  Although I would prefer if you didn't file duplicate bug reports, don't
        feel like you need to exhaustively examine every bug report before submitting
        one. 
.. [#macro-hygiene]  
    If you would prefer not to have lowercase macros polluting the global
    namespace, just put ``#define AB_NO_KEYWORDS`` at the top of the file. You
    can still use Autobind by using ``AB_EXPORT``, ``AB_MODULE()``, and so on
    instead of ``pyexport`` and ``pymodule``.

    The ``py*`` variants are intended to make the purpose of the directives more
    obvious to those who are not familiar with Autobind. They are lowercase to 
    make them easier to type, and to avoid collisions with Python/C API functions
    and macros, which all begin with ``Py*``.
