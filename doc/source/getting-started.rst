
===============
Getting Started
===============

Building Autobind
-----------------

.. raw:: html
    
    <div class="pull-right">
        <a class="btn btn-primary" href="https://bitbucket.org/saroth/autobind/downloads">
            <i class="fa fa-download"></i>&nbsp;Get the Source
        </a>
    </div>

:you'll need:
    • CMake
    • Clang ≥ 3.4 (with headers)
    • LLVM libc++
    • Boost (headers only)
    • Python ≥ 3.3 (for running the wrapper script)

.. highlight:: console

If Clang is not your default compiler you must set the CXX environment variable
to your Clang binary.

.. note::

    A note and apology to GNU users:

    First, you may be able to use GCC 4.9 to build autobind, but I haven't
    tested this. The alternative is somewhat more complicated. (I haven't had
    time to test this either, but I think that it may work.)

    Clang itself must be built using libc++ (rather than libstdc++) or
    you will receive errors upon trying to link. If you do, download the Clang and
    LLVM sources, configure them to be built using an existing Clang and libc++,
    then build Clang and LLVM. Finally, when attempting to rebuild autobind, make
    sure these libraries are the ones the autobind finds during its configuration
    process. You may need to alter the CMake variables manually. Consult the CMake
    manual.

    Yes, this process involves three compilers and two standard libraries. I'm
    sorry. There's nothing to be done about this, besides rewrite the parts of the
    codebase that use C++1y. which is what I will be doing over the next few days.

    For the time being, if you have a Mac or a recent FreeBSD system (both LLVM-based),
    consider using that instead.


Once you've installed the dependencies, just do::

    $ hg clone https://saroth@bitbucket.org/saroth/autobind
    $ cd autobind
    $ cmake .
    $ make -j4

If you get errors about ``auto`` not being valid in function declarations,
there is likely a problem with your compiler. Please see the note above.

If it doesn't work, you found a bug, either in the program or the
documentation. Please file a bug report [#bugs]_.


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
    Python/C modules and not just those produced by Autobind.
    

Next comes the easy part. Open up a terminal and type::
    
    $ /path/to/autobind/scripts/autobind.py build -c hello.cpp

.. highlight:: ipython

Then, open up a Python3 shell and type::
    

    In [1]: import hello
    In [2]: hello.hello_world()
    Hello, world!



.. rubric:: Footnotes


.. [#bugs]  Although I would prefer if you didn't file duplicate bug reports, don't
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

