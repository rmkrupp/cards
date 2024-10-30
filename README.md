# cards
a card game engine for a card game

Build with `./configure.py && ninja`

## Dependencies

### Build

 - gcc etc.
 - ninja
 - python
 - gperf

### Runtime
 - libevent
 - either Lua 5.1 or luajit

## W64

Use `./configure.py --build=w64`.

### ArchLinux instructions

Install the `mingw-w64` group.

AUR packages needed:

 - `mingw-w64-zlib`
 - `mingw-w64-environment`
 - `mingw-w64-openssl`
 - `mingw-w64-pkg-config`
 - `mingw-w64-libevent`
 - `mingw-w64-luajit`

The build is static. DLLs for libevent, luajit, winpthreads, etc. don't need
to be distributed with the executables.

(See the helper scripts in `misc/pkg/archlinux/w64`.)

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
