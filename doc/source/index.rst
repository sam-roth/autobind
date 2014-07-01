.. Autobind documentation master file, created by
   sphinx-quickstart on Tue Jun 24 18:00:26 2014.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.


.. raw:: html
    
    <div style="padding-top: 24pt;"></div>




.. rst-class:: jumbotron
.. container::

    Autobind is a `Clang-based <http://clang.llvm.org/>`_ tool that
    automatically generates Python bindings for C++ code.

    Type this:

    .. code-block:: cpp

        /// Compute the value of A(m, n), the Ackermann function.
        int pyexport ackermann(int m, int n) {
            if(m < 0 || n < 0) 
                throw std::out_of_range("m and n must be non-negative");
            return m == 0? n + 1 
            :      n == 0? ackermann(m - 1, 1)
            :              ackermann(m - 1, ackermann(m, n - 1));
        }

    Get this:
    
    .. code-block:: pycon

        >>> from ackermann import ackermann
        >>> print(ackermann.__doc__)
        (m: int, n: int) -> int
        Compute the value of A(m, n), the Ackermann function.
        >>> [((m, n), ackermann(m, n)) for m in range(4) for n in range(5)]
        [((0, 0), 1), 
         ...
         ((3, 4), 125)]
        >>> ackermann(-1, 0)
        RuntimeError                              Traceback (most recent call last)
          ...
        RuntimeError: m and n must be non-negative
        
        


Contents:

.. toctree::
    :maxdepth: 2

    getting-started
    cxx-support
    autobind-cxxapi
    implementation-notes


Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

