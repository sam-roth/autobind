


import abutil
import os.path

def check_errors(source, errors):
	def error_set(e):
		return set(tuple(sorted(x.items())) for x in e)
	expected = error_set(errors)
	try:
		abutil.run_autobind(source, capture_errors=True)
	except abutil.BindingGenerationFailedError as exc:
		actual = error_set(exc.args[0])
	else:
		actual = set([])

	assert actual == expected


def test_fail_no_conversion():
	file = os.path.abspath('fail_no_conversion.cpp')

	expected = [dict(filename=file, message="No specialization of python::Conversion for type 'class Foo'",
	                 line=16, col=30),
	            dict(filename=file, message="No specialization of python::Conversion for type 'class Foo'",
	                 line=11, col=14)]

	check_errors(file, expected)


def test_fail_getter_arity():
	file = os.path.abspath('fail_getter_arity.cpp')
	expected=[dict(filename=file, message='getter must have no parameters', line=15, col=23)]
	check_errors(file, expected)
def test_fail_setter_arity():
	file = os.path.abspath('fail_setter_arity.cpp')
	expected=[dict(filename=file, message='setter must have exactly one parameter', line=15, col=19)]
	check_errors(file, expected)
def test_fail_getter_result():
	file = os.path.abspath('fail_getter_result.cpp')
	expected=[dict(filename=file, message='getter must not return `void`.', line=15, col=19)]
	check_errors(file, expected)

