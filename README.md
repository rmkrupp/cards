# cards
a card game engine for a card game

## Dependencies

### Build

 - gcc etc.
 - ninja
 - gperf

### Runtime
 - libevent
 - either Lua 5.1 or luajit

## W64

Use `./configure release-w64-luajit` then run `ninja`. 

### ArchLinux instructions
Install the `mingw-w64` group.

AUR packages needed:

 - `mingw-w64-environment`
 - `mingw-w64-libevent`
 - `mingw-w64-luajit`
 - `mingw-w64-openssl`
 - `mingw-w64-pkg-config`
 - `mingw-w64-zlib`

The build is static. DLLs for libevent, luajit, winpthreads, etc. don't need
to be distributed with the executables.
