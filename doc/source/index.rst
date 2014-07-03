.. Autobind documentation master file, created by
   sphinx-quickstart on Tue Jun 24 18:00:26 2014.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. raw:: html
    
    <div style="padding-top: 4ex;"></div>

.. rst-class:: jumbotron
.. container::

    Autobind is a Clang-based tool that
    automatically generates Python bindings for C++ code.

    Type this:

    .. code-block:: cpp

        /// Compute the value of A(m, n), the Ackermann function.
        int pyexport ackermann(int m, int n) {
            if(m < 0 || n < 0) 
                throw std::domain_error("m and n must be non-negative");
            return m == 0? n + 1 
            :      n == 0? ackermann(m - 1, 1)
            :              ackermann(m - 1, ackermann(m, n - 1));
        }

    Get this:
    
    .. code-block:: ipython

        In [1]: from ackermann import ackermann
        In [2]: ackermann?
        ...
        (m: int, n: int) -> int
        Compute the value of A(m, n), the Ackermann function.

        In [3]: [((m, n), ackermann(m, n)) for m in range(4) for n in range(5)]
        Out[3]:
        [((0, 0), 1), ..., ((3, 4), 125)]
        In [4]: ackermann(-1, 0)
        ---------------------------------------------------------------------------
        RuntimeError                              Traceback (most recent call last)
        ...
        RuntimeError: m and n must be non-negative
        
    .. raw:: html
        
        <p "style: margin-top:15px;" class="text-right">
            <a class="btn btn-primary btn-lg" role="button" href="./getting-started.html">
                Get Started
                <span class="glyphicon glyphicon-chevron-right"></span>
            </a>
        </p>
        

Table of Contents
=================

.. toctree::
    :maxdepth: 2

    getting-started
    cxx-support
    autobind-cxxapi
    appendices


Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

Further Reading
===============

.. `Python/C API <https://docs.python.org/3/c-api/>`_

* Technologies
    * `Clang <http://clang.llvm.org>`_
    * Clang `LibTooling <http://clang.llvm.org/docs/LibTooling.html>`_
* Similar Projects
    * `MOC-NG <http://woboq.com/blog/moc-with-clang.html>`_, a Clang-based
      reimplementation of the Qt Meta-Object Compiler.

.. _other-tools:

* Other Ways of Creating Native-Code Python Modules
    * Official
        * :ref:`The Python/C API <python:c-api-index>`
        * :py:mod:`ctypes`
    * Unofficial
        * `CFFI <https://cffi.readthedocs.org/en/release-0.8/>`_
        * `SWIG <http://www.swig.org/>`_ (also supports languages other than Python)
        * `Cython <http://www.cython.org/>`_, a Python dialect with builtin C and C++ compatibility
        * `Boost.Python <http://www.boost.org/doc/libs/1_55_0/libs/python/doc/index.html>`_, a pure-C++ approach


