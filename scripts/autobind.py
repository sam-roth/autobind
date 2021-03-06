#!/usr/bin/env python3


import tempfile
import subprocess
import shlex
import os.path
import sysconfig
import itertools
import argparse
import contextlib
import os
import sys

CXX = os.environ.get('CXX', 'c++')


AB = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                  os.path.pardir,
                  'bin',
                  'autobind')
if not os.path.exists(AB):
    AB = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                      os.path.pardir,
                      'build',
                      'bin',
                      'autobind')

AB_INCLUDE = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                          os.path.pardir,
                          'include')


LIBCPP_MAVERICKS_WORKAROUND_PATH = '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../include/c++/v1'



CXXFLAGS = ['-std=c++11',
            '-stdlib=libc++',
            '-I' + AB_INCLUDE]

if os.path.exists(LIBCPP_MAVERICKS_WORKAROUND_PATH):
    CXXFLAGS += ['-isystem', LIBCPP_MAVERICKS_WORKAROUND_PATH]

# Suppress unused argument warning: This script isn't smart enough to figure
# out which arguments autobind will use, so it provides it all of them.
# This isn't much of a risk, considering that Clang will point out the unused arguments
# during the build phase.
ADDITIONAL_DRIVER_FLAGS = ['-Qunused-arguments'] #, '-Wno-error=unused-command-line-argument-hard-error-in-future']

def get_cc1_flags(cflags):

    with subprocess.Popen([CXX, '-###'] + cflags
                          + cflags,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE) as p:
        stdout, stderr = p.communicate()

        stderr = stderr.decode()


        if p.returncode != 0:
            raise RuntimeError('Clang returned non-zero status %r.' % p.returncode)

    for line in stderr.splitlines():
        if '-cc1' in line:
            return ADDITIONAL_DRIVER_FLAGS + list(itertools.takewhile(lambda x: x != '-o', shlex.split(line)))
    else:
        raise RuntimeError("Couldn't infer cc1 flags from clang output:\n%s" % stderr)




def get_python_cflags():
    return ['-I' + sysconfig.get_path('include'),
            '-I' + sysconfig.get_path('platinclude')]

def get_python_ldflags():
    pyver = sysconfig.get_config_var('VERSION')
    getvar = sysconfig.get_config_var
    libs = getvar('LIBS').split() + getvar('SYSLIBS').split()
    libs.append('-lpython' + pyver + sys.abiflags)
    # add the prefix/lib/pythonX.Y/config dir, but only if there is no
    # shared library in prefix/lib/.
    if not getvar('Py_ENABLE_SHARED'):
        libs.insert(0, '-L' + getvar('LIBPL'))
    if not getvar('PYTHONFRAMEWORK'):
        libs.extend(getvar('LINKFORSHARED').split())
    return libs

def get_autobind_flags(source):

    return get_cc1_flags(CXXFLAGS + get_python_cflags() + ['-DAUTOBIND_RUN']
                         + ['-c', source])

class BindingGenerationFailedError(RuntimeError):
    pass

def run_autobind(source):
    source = os.path.abspath(source)
    flags = get_autobind_flags(source)

    with subprocess.Popen([AB, source, '--'] + flags,
                          stdout=subprocess.PIPE) as proc:
        stdout, _ = proc.communicate()
        if proc.returncode != 0:
            raise BindingGenerationFailedError('Binding generation failed.')
        return stdout.decode()



        

def main():



    ap = argparse.ArgumentParser()

    def show_help(args):
        if 'verb' in args:
            if args.verb == 'generate':
                generate_v.print_help()
            elif args.verb == 'build':
                print('todo')
            else:
                sys.stderr.write('Unknown verb: %r' % args.verb)
                ap.print_help()
        else:
            ap.print_help()

    def generate(args):
        global CXXFLAGS
        with args.o:
            CXXFLAGS += args.cxxflags
            args.o.write(run_autobind(args.c))

    def build(args):
        global CXXFLAGS
        main_file, *other_files = args.c

        with tempfile.NamedTemporaryFile('w', dir=os.path.dirname(main_file), suffix='.cpp') as f:
            CXXFLAGS += args.cxxflags
            try:
                f.write(run_autobind(main_file))
            except BindingGenerationFailedError:
                sys.exit(1)
            f.flush()

            stem, ext = os.path.splitext(os.path.basename(main_file))

            lib_suffix = sysconfig.get_config_var('EXT_SUFFIX')

            path = args.o
            if path is None:
                path = os.path.join(os.path.dirname(main_file),
                                    stem + lib_suffix)


            subprocess.check_call([CXX] + CXXFLAGS
                                  + get_python_cflags()
                                  + get_python_ldflags()
                                  + other_files
                                  + [f.name]
                                  + ['-shared', '-o', path])
                                                



    verbs = ap.add_subparsers(metavar='VERB')

    help_ = verbs.add_parser('help', help='show the help for the verb')
    help_.add_argument('verb', help='show the help message', nargs='?')
    help_.set_defaults(func=show_help)


    generate_v = verbs.add_parser('generate', help='generate binding code')
    generate_v.add_argument('-c', help='source file', required=True)
    generate_v.set_defaults(func=generate)
    generate_v.add_argument('-o', help='output file (default is stdout)', type=argparse.FileType('w'), default='-')

    build_v = verbs.add_parser('build', help='generate binding code and compile it into a shared library')
    build_v.add_argument('-c', help='source files (the first is used as the autobind input)', required=True, action='append')
    build_v.add_argument('-o', help='output file', type=str, default=None)
    build_v.set_defaults(func=build)

    def add_cxxflags(p):
        p.add_argument('cxxflags', help='Additional compiler flags. You must prefix the first '
                                        'flag in this list with --, as in `autobind.py -c foo.cpp -- -I/bar/include`',
                       nargs='*')

    add_cxxflags(generate_v)
    add_cxxflags(build_v)

    args = ap.parse_args()
    if 'func' in args:
        args.func(args)
    else:
        show_help(args)




if __name__ == '__main__':
    main()

