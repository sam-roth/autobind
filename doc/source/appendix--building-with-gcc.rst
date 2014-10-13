
.. _appendix--building-with-gcc:

===========================
Appendix: Building with GCC
===========================

:abstract:
    On GNU/Linux and other GNU systems, it may be desirable to build Autobind using GCC
    and libstdc++, rather than Clang and libc++. This is due to the fragile binary interface
    problem: Attempting to use premade binaries built using one toolchain with a different 
    toolchain may cause linking errors, or worse, especially when working with C++ code.


Compilation Procedure
=====================


:you'll need:   • Clang ≥ 3.5 (with headers)
                • GCC ≥ 4.8
                • Boost.Regex (libstdc++ has no regex support [≤ 4.8], so we'll use Boost as a standin)
                • Python ≥ 3.3 (with headers)
                • CMake

.. note::
    
    If you're using Ubuntu, you can install these dependencies with::
        
        $ sudo apt-get install build-essential libclang-3.5-dev clang-3.5 gcc-4.8 \
            libboost-regex-dev cmake libpython3.4-dev python3.4-dev python3.4

1. Install the dependencies
2. Clone the repository::

    $ hg clone https://saroth@bitbucket.org/saroth/autobind
    $ cd autobind
    
3.  Create a file called :file:`CMakeLists.user.cmake` in the root of the repository.
    Note the extension: ``.cmake``, rather than ``.txt``, which is typical for CMakeLists
    files.

    Put the following directives into the file and save it:

    .. code-block:: cmake
        
        set(STDLIB_FLAG "-DAB_NO_STD_REGEX")
        set(EXTRA_LIBS dl pthread boost_regex tinfo)

    The first line defines a preprocessor macro that causes Autobind to include
    ``<boost/tr1/regex.hpp>`` in place of ``<regex>``, and to bring those names
    into scope. The second line directs the linker to link with several extra
    libraries, including ``boost_regex``, that are necessary when working on a
    GNU system.
        
4. Continue with the build procedure as normal::

    $ cmake .
    $ make -j4
    
Using the Wrapper Script in a Mixed Clang/GCC Environment
=========================================================

Currently, the wrapper script only supports using Clang as a compiler. When you
use the wrapper script, you must set the ``CXX`` environment variable to point
to Clang. You may use a directive like the following to temporarily set this
environment variable for the duration of the script::

    $ CXX=clang++ /path/to/autobind.py build -c mymodule.cpp

If you're using Fish, rather than a Bourne-like shell, the equivalent command is::
    
    > env CXX=clang++ /path/to/autobind.py build -c mymodule.cpp



