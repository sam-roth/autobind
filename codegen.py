#!/usr/bin/env python3

import subprocess, shlex, os.path
import os
import itertools

cflags = shlex.split('-I/opt/local/include '
                     '-I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include '
                     '-I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../li'
                     'b/clang/5.1/include '
                     '-I/usr/include '
                     '-I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/c++/v1 '
                     '-std=c++11')



def get_py_cflags():
	result = shlex.split(subprocess.getoutput('python3.3-config --cflags'))
	return result


def get_py_ldflags():
	result = shlex.split(subprocess.getoutput('python3.3-config --ldflags'))
	try:
		result.remove('-lintl')
	except ValueError:
		pass

	return result

py_cflags = get_py_cflags()
py_ldflags = get_py_ldflags()


def build(infile='example.cpp'):
	prefix, suffix = os.path.splitext(infile)

	with open(prefix + '.bind.cpp', 'wb') as f:

		debugger = ['lldb', '--'] if os.environ.get('DEBUG') == '1' else []
		
		args = itertools.chain(debugger,
		                       ['./bin/autobind', infile, '--', '-c'],
		                       cflags,
		                       py_cflags,
		                       ['-DAUTOBIND_RUN'])
		kwargs = dict() if debugger else dict(stdout=f)


		subprocess.call(list(args), **kwargs)

# 		subprocess.call(['./bin/autobind', infile, '--', '-c'] + cflags + py_cflags + ['-DAUTOBIND_RUN'], stdout=f)


	subprocess.check_call(['clang++'] + cflags + py_cflags + py_ldflags + ['-shared', prefix + '.bind.cpp', '-o', 
	                                                                       prefix + '.so'])


if __name__ == '__main__':
	import sys
	if len(sys.argv) > 1:
		for item in sys.argv[1:]:
			build(item)
	else:
		build()

