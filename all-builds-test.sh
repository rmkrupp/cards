#!/bin/bash
# File: all-builds-test.sh
# Part of cards <github.com/rmkrupp/cards>
#
# Copyright (C) 2024 Noah Santer <n.ed.santer@gmail.com>
# Copyright (C) 2024 Rebecca Krupp <beka.krupp@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


function build() {
    if ./configure.py $@ && ninja ; then
        echo "build '$@' passed"
    else
        echo "build '$@' failed"
    fi

}

# pre-clean

ninja -tclean

# debug builds

build
build --lua-backend=lua51
build --lua-backend=none
build --enable-compatible
build --disable-server
build --disable-argp
build --disable-verbose-lexer
build --enable-hash-statistics
build --disable-hash-warnings
build --no-defer-pkg-config
build --ldflags="-Wl,--as-needed"

# release builds

build --build=release
build --O3 --build=release
build --lua-backend=lua51 --build=release
build --lua-backend=none --build=release
build --enable-compatible --build=release
build --enable-compatible --O3 --build=release
build --disable-server --build=release
build --disable-argp --build=release
build --disable-verbose-lexer --build=release
build --enable-hash-statistics --build=release
build --disable-hash-warnings --build=release
build --no-defer-pkg-config --build=release
build --ldflags="-Wl,--as-needed" --build=release

# w64 builds

build --build=w64
build --enable-compatible --build=w64
#build --lua-backend=lua51 --build=w64
build --lua-backend=none --build=w64
build --disable-server --build=w64
#build --disable-argp --build=w64
build --disable-verbose-lexer --build=w64
build --enable-hash-statistics --build=w64
build --disable-hash-warnings --build=w64
build --no-defer-pkg-config --build=w64
build --ldflags="-Wl,--as-needed" --build=w64

# clean up a bit
./configure.py && ninja -tclean
./configure.py --build=w64 && ninja -tclean
./configure.py
