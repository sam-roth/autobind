#!/usr/bin/env python3

# PYCFLAGS=$(python3.3-config --cflags)

# ./bin/autobind example.cpp -- -c $PYCFLAGS -I/opt/local/include -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/clang/5.1/include -I/usr/include -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/c++/v1 -std=c++11  > example.bind.cpp


# clang++ -I/opt/local/include -I/opt/local/Library/Frameworks/Python.framework/Versions/3.3/include/python3.3m -I/opt/local/Library/Frameworks/Python.framework/Versions/3.3/include/python3.3m -Wno-unused-result -fno-common -dynamic -DNDEBUG -g -fwrapv -O3 -Wall -Wstrict-prototypes -pipe -Os -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk -arch x86_64 -L/opt/local/Library/Frameworks/Python.framework/Versions/3.3/lib/python3.3/config-3.3m  -ldl -framework CoreFoundation -lpython3.3m -shared -std=c++11 example.bind.cpp -o example.so

import subprocess, shlex, os.path

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
		subprocess.check_call(['./bin/autobind', infile, '--', '-c'] + cflags + py_cflags, stdout=f)


	subprocess.check_call(['clang++'] + cflags + py_cflags + py_ldflags + ['-shared', prefix + '.bind.cpp', '-o', 
	                                                                       prefix + '.so'])


if __name__ == '__main__':
	import sys
	if len(sys.argv) > 1:
		for item in sys.argv[1:]:
			build(item)
	else:
		build()

