#!/bin/bash

function build() {
    if ./configure.py $@ && ninja ; then
        echo "build '$@' passed" | tee log -a
    else
        echo "build '$@' failed" | tee log -a
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
