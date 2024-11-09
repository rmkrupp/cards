#/bin/bash

#
# uninstall the AUR packages installed by install.sh
#

sudo pacman --noconfirm -R mingw-w64-libevent
sudo pacman --noconfirm -R mingw-w64-luajit
sudo pacman --noconfirm -R mingw-w64-sqlite
sudo pacman --noconfirm -R mingw-w64-libunistring
sudo pacman --noconfirm -R mingw-w64-libiconv
sudo pacman --noconfirm -R mingw-w64-openssl
sudo pacman --noconfirm -R mingw-w64-zlib
sudo pacman --noconfirm -R mingw-w64-configure
sudo pacman --noconfirm -R mingw-w64-readline
sudo pacman --noconfirm -R mingw-w64-pdcurses
sudo pacman --noconfirm -R mingw-w64-termcap
sudo pacman --noconfirm -R mingw-w64-environment
sudo pacman --noconfirm -R mingw-w64-pkg-config
