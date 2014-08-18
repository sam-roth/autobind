
import sys
import os.path
import os

here = os.path.abspath(os.path.dirname(__file__))

scripts_dir = os.path.join(here,
                           os.path.pardir,
                           'scripts')

sys.path.append(scripts_dir)

import autobind
import contextlib
import tempfile
import yaml

@contextlib.contextmanager
def env(**kw):
	'''
	Context manager that sets the given environment variables on entry
	and restores them upon exit.

	>>> with env(VAR='foo'):
	...     with env(VAR='bar'):
	...         print(os.environ['VAR'])
	...     print(os.environ['VAR'])
	bar
	foo
	'''
	sentinel = object()
	saved = {k: os.environ.get(k, sentinel) for k in kw}

	try:
		os.environ.update(**kw)
		yield
	finally:
		for k, v in saved.items():
			if v is sentinel:
				try:
					del os.environ[k]
				except KeyError:
					pass
			else:
				os.environ[k] = v



BindingGenerationFailedError = autobind.BindingGenerationFailedError





def _run_autobind_with_errors(source):
	with tempfile.NamedTemporaryFile('w+') as tf:
		with env(AB_EMIT_YAML_DIAG=str(tf.name)):
			try:
				return autobind.run_autobind(source)
			except autobind.BindingGenerationFailedError as exc:
				raise autobind.BindingGenerationFailedError(list(yaml.load_all(tf)))




def run_autobind(source, *, capture_errors=False):
	if not capture_errors:
		return autobind.run_autobind(source)
	else:
		return _run_autobind_with_errors(source)







