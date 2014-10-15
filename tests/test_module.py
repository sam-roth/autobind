
import module
import pytest

def test_constructor():
	# TODO: constructor docstrings
	k = module.Keywords(1, 'abcd', 'efg')
	assert k.foo == 1
	assert k.bar == 'abcd'
	assert k.baz == 'efg'

def test_constructor_kw():
	k = module.Keywords(foo=1,
	                    bar='abcd',
	                    baz='efg')

	assert k.foo == 1
	assert k.bar == 'abcd'
	assert k.baz == 'efg'


def test_call():
	k = module.make_keywords(1, 'abcd', 'efg')

	assert k.foo == 1
	assert k.bar == 'abcd'
	assert k.baz == 'efg'


def test_call_kw():
	k = module.make_keywords(foo=1, 
	                         bar='abcd',
	                         baz='efg')

	assert k.foo == 1
	assert k.bar == 'abcd'
	assert k.baz == 'efg'

def test_call_some_kw():
	k = module.make_keywords(1,
	                         bar='abcd',
	                         baz='efg')

	assert k.foo == 1
	assert k.bar == 'abcd'
	assert k.baz == 'efg'

def test_allocs():
	module.reset_allocs()
	acs = [module.AllocCheck() for _ in range(10)]
	assert module.alloc_count() == 10 and module.dealloc_count() == 0
	del acs
	assert module.alloc_count() == 10 and module.dealloc_count() == 10


def test_throw_alloc():
	module.reset_allocs()
	for i in range(100):
		try:
			module.ThrowCheck()
		except RuntimeError as exc:
			assert str(exc) == 'test'

	assert module.alloc_count() == module.dealloc_count() and module.alloc_count() == 100


def test_func_docstring():
	assert module.docstring_test_1.__doc__.strip() == '()\ndocstring test 1'
	assert module.docstring_test_2.__doc__.strip() == ('(i: int, j: std::string) -> int'
	                                                   '\ndocstring test 2')
def test_class_docstring():
	assert module.DocstringTest3.__doc__.strip() == 'docstring test 3'
	assert module.DocstringTest3.docstring_test_4.__doc__.strip() == (
		'(i: int) -> int\ndocstring test 4')

def test_using():
	assert module.double_number(1) == 2
	assert module.TestUsing() is not None

def test_overload():
	# pyexport std::string overload() { return ""; }
	assert module.overload() == ''
	# pyexport std::string overload(int) { return "int"; }
	assert module.overload(1) == 'int'
	# pyexport std::string overload(const std::string &) { return "std::string"; }
	assert module.overload('') == 'std::string'
	# pyexport std::string overload(int, int) { return "int, int"; }
	assert module.overload(1, 1) == 'int, int'

def test_constructor_overload():
	assert module.ConstructorOverload().get() == 0
	assert module.ConstructorOverload(1).get() == 1
	assert module.ConstructorOverload(1,1).get() == 2


def test_field_getters():
	acc = module.Accessors(1234)
	assert acc.foo == 1234

def test_field_setters():
	acc = module.Accessors(0)
	acc.foo = 5678
	assert acc.foo == 5678

def test_field_setter_validates_type():
	acc = module.Accessors(0)

	with pytest.raises(TypeError):
		acc.foo = 'abcd'

def test_field_docstring():
	assert module.Accessors.foo.__doc__.strip() == 'docstring for foo'

def test_methods():
	m = module.Methods('baz')
	assert m.foo() == 'foobaz'
	assert m.bar() == 'barbaz'
