#/bin/bash

#
# build and install the AUR packages needed to cross-compile
#

function fetch_build() {
    pacman -Qq "$1" && return
    git clone "https://aur.archlinux.org/$1" "$1.gen" || exit 1
    cd "$1.gen" || exit 1
    makepkg --log && \
        sudo pacman --noconfirm -U *.pkg.tar.zst || { cd .. ; exit 1 ; }
    cd ..
}

#configure (and openssl) depends on...
fetch_build mingw-w64-environment
fetch_build mingw-w64-pkg-config

#readline (and sqlite) depends on...
fetch_build mingw-w64-configure

#termcap depends on...
fetch_build mingw-w64-pdcurses

#readline depends on...
fetch_build mingw-w64-termcap

# sqlite depends on...
fetch_build mingw-w64-readline

# openssl depends on...
fetch_build mingw-w64-zlib

# libevent depends on...
fetch_build mingw-w64-openssl

# libuninstring depends on...
fetch_build mingw-w64-libiconv

# cards depends on...
fetch_build mingw-w64-luajit
fetch_build mingw-w64-libevent
fetch_build mingw-w64-sqlite
fetch_build mingw-w64-libunistring
