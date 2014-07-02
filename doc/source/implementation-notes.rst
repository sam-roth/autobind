




Implementation Notes
====================

.. note::
    This page is a work in progress.

Annotation "Keywords"
---------------------

All of the "keywords" that Autobind uses are defined as macros. These expand to
GNU-style ``__attribute__`` annotations. The particular attribute used is
Clang's ``annotate`` attribute, which permits the attachment of an arbitrary
string to an AST node [#mocng]_ . The definition of these macros is shown below:

.. code-block:: c++

    #define AB_PRIVATE_ANNOTATE(a...)        __attribute__((annotate(a)))
    #define AB_PRIVATE_TU_ANNOTATE(a...)     namespace AB_PRIVATE_ANNOTATE(a) { }

    #define AB_EXPORT                        AB_PRIVATE_ANNOTATE("pyexport")
    #define AB_MODULE(name)                  AB_PRIVATE_TU_ANNOTATE("py:module:" #name)
    #define AB_DOCSTRING(text)               AB_PRIVATE_TU_ANNOTATE("pydocstring:" text)
    #define AB_GETTER(name)                  AB_PRIVATE_ANNOTATE("pygetter:" #name)
    #define AB_SETTER(name)                  AB_PRIVATE_ANNOTATE("pysetter:" #name)
    #define AB_NOEXPORT                      AB_PRIVATE_ANNOTATE("pynoexport")

    #ifndef AB_NO_KEYWORDS
    #   define pyexport    AB_EXPORT
    #   define pymodule    AB_MODULE
    #   define pydocstring AB_DOCSTRING
    #   define pygetter    AB_GETTER
    #   define pysetter    AB_SETTER
    #endif


The Autobind Code Generator
---------------------------

Extraction of Interface from AST
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Autobind uses a custom AST consumer which invokes the ``DiscoveryVisitor``
module to extract the interface of all suitably-annotated bindable names.

.. Code Generation
.. ^^^^^^^^^^^^^^^


Other Notes
-----------

Why not use the C++ ``extern`` keyword, as in ``extern "Python"``?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This is actually what I wanted to do, but it seems that doing this would have required 
modifying the Clang parser to preserve this information, rather than discarding it as an
invalid linkage type. It also would have made it necessary to strip the annotations from
the source code before passing them to the actual compiler, since Clang will emit an
error on an unknown ``extern`` linkage type::

    a.cpp:2:8: error: unknown linkage language
    extern "Python" void foo() { }
           ^

As will GCC::

    a.cpp:2:8: error: language string '"Python"' not recognized
     extern "Python" void foo() { }
            ^

.. rubric:: Footnotes

.. [#mocng] This technique is also used by 
            `MOC-NG <http://woboq.com/blog/moc-with-clang.html>`_, 
            a Clang-based reimplementation of the Qt `Meta-Object Compiler <http://qt-project.org/doc/qt-5/moc.html#moc>`_.
            It has also been `suggested by Doug Gregor <http://llvm.org/devmtg/2011-11/Gregor_ExtendingClang.pdf>`_
            as part of his "Extending Clang" talk at the `2011 LLVM Developers' Meeting <http://llvm.org/devmtg/2011-11/>`_.

