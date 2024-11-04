#!/bin/bash

function build() {
    if ./configure.py $@ && ninja ; then
        echo "build '$@' passed"
    else
        echo "build '$@' failed"
    fi

}

build
build --lua-backend=lua51
build --lua-backend=none
build --disable-server
build --disable-readline
build --disable-argp
build --disable-verbose-lexer
build --enable-hash-statistics
build --disable-hash-warnings

build --build=release
build --lua-backend=lua51 --build=release
build --lua-backend=none --build=release
build --disable-server --build=release
build --disable-readline --build=release
build --disable-argp --build=release
build --disable-verbose-lexer --build=release
build --enable-hash-statistics --build=release
build --disable-hash-warnings --build=release

build --build=w64
#build --lua-backend=lua51 --build=w64
build --lua-backend=none --build=w64
build --disable-server --build=w64
#build --disable-readline --build=w64
#build --disable-argp --build=w64
build --disable-verbose-lexer --build=w64
build --enable-hash-statistics --build=w64
build --disable-hash-warnings --build=w64


# Clean up a bit
./configure.py && ninja -tclean
./configure.py --build=w64 && ninja -tclean
./configure.py
