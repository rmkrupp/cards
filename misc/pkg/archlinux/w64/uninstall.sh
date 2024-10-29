#/bin/bash

#
# uninstall the AUR packages installed by install.sh
#

sudo pacman --noconfirm -R mingw-w64-libevent
sudo pacman --noconfirm -R mingw-w64-luajit
sudo pacman --noconfirm -R mingw-w64-openssl
sudo pacman --noconfirm -R mingw-w64-pkg-config
sudo pacman --noconfirm -R mingw-w64-environment
sudo pacman --noconfirm -R mingw-w64-zlib

