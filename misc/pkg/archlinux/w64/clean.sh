#/bin/bash

#
# remove the local directories created by install.sh
#

function clean() {
    rm -rf "$1.gen"
}

clean mingw-w64-pkg-config
clean mingw-w64-configure
clean mingw-w64-environment
clean mingw-w64-libevent
clean mingw-w64-luajit
clean mingw-w64-openssl
clean mingw-w64-pdcurses
clean mingw-w64-readline
clean mingw-w64-sqlite
clean mingw-w64-termcap
clean mingw-w64-winpthreads
clean mingw-w64-zlib
