#/bin/bash

#
# build and install the AUR packages needed to cross-compile
#

function fetch_build_pkgctl() {
    pacman -Qq "$1" && return
    git clone "https://aur.archlinux.org/$1" "$1.gen" || exit 1
    cd "$1.gen" || exit 1
    makepkg --log && \
        sudo pacman --noconfirm -U *.pkg.tar.zst || { cd .. ; exit 1 ; }
    cd ..
}

# openssl depends on...
fetch_build_pkgctl mingw-w64-zlib
fetch_build_pkgctl mingw-w64-environment

# libevent depends on...
fetch_build_pkgctl mingw-w64-openssl
fetch_build_pkgctl mingw-w64-pkg-config

fetch_build_pkgctl mingw-w64-luajit
fetch_build_pkgctl mingw-w64-libevent
