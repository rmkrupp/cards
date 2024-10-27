# cards
a card game engine for a card game

## Dependencies

 - libevent
 - either Lua 5.1 or luajit

## W64

Use `./configure release-w64-luajit`

Install the `mingw-w64` group.

AUR packages needed:

 - `mingw-w64-environment`
 - `mingw-w64-libevent`
 - `mingw-w64-luajit`
 - `mingw-w64-openssl`
 - `mingw-w64-pkg-config`
 - `mingw-w64-zlib`

The build is static for DLLs for libevent, luajit, winpthreads, etc. don't need
to be distributed with the executables.
