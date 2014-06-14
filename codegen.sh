#!/bin/bash

PYCFLAGS=$(python3.3-config --cflags)

./bin/hgr example.cpp -- -c $PYCFLAGS -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/clang/5.1/include -I/usr/include -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/c++/v1 -std=c++11  > example.bind.cpp


clang++ -I/opt/local/Library/Frameworks/Python.framework/Versions/3.3/include/python3.3m -I/opt/local/Library/Frameworks/Python.framework/Versions/3.3/include/python3.3m -Wno-unused-result -fno-common -dynamic -DNDEBUG -g -fwrapv -O3 -Wall -Wstrict-prototypes -pipe -Os -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk -arch x86_64 -L/opt/local/Library/Frameworks/Python.framework/Versions/3.3/lib/python3.3/config-3.3m  -ldl -framework CoreFoundation -lpython3.3m -shared -std=c++11 example.bind.cpp -o example.so