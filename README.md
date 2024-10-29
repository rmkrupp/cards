# cards
a card game engine for a card game

## tl;dr

Makefile wrapper: `DESTDIR=/where/to/install/besides/usr/local ./configure && make && make install`

or

`DESTDIR=/as/above ./configure && ninja && ninja install`

*This is not really autotools, this configure ignores options you might pass
it!*

Instead, you can `./configure BUILDTYPE` where `BUILDTYPE` matches one of the
files in the `ninja/` directory, i.e.:

 - `debug-luajit`: build for debug (`-g -Og` plus sanitizers) with luajit
 - `debug-lua51`: build for debug (`-g -Og` plus sanitizers) with plain lua
 - `release-luajit`: build for release with luajit
 - `release-lua51`: build for release with plain lua
 - `release-w64-luajit`: cross-compile with mingw (see below) with luajit

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

## Project Goals

A non-exhaustive list...

 - [ ] Functional server
   - [ ] Parses commands
   - [ ] Loading of rules from configuration
   - [ ] Manages game state
   - [ ] Manages lobby vs in-game and player assignment
   - [ ] Manages reconnection and save/load
   - [ ] Provides API for cards
   - [ ] Loads cards and related assets
 - [ ] An asset wrangler
   - [ ] Takes card scripts and data and packages it up
 - [ ] Basic text client
   - [ ] readline-like convenience (history, tab completion)
   - [ ] Colorized
 - [ ] Graphical client
 - [ ] Computer-controlled client
 - [ ] Wrapper for easy setup of common play scenarios
 - [ ] Packages for
   - [ ] ArchLinux
   - [ ] Debian
   - [ ] Windows
 - [ ] Testing, especially for rigorous handling of inputs to server
 - [ ] Our personal card library and pre-defined decks, including flavor
