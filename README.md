# cards
a card game engine for a card game

This project has submodules. When you clone it run `git submodule update
--init`.

Build with `./configure.py --build=release && ninja`

## Dependencies

### Build

 - gcc etc.
 - ninja
 - python
 - gperf

### Runtime

 - libevent
 - either Lua 5.1 or luajit
 - sqlite3
 - readline (unless `--disable-readline`, which stops `rlcli` from building)
 - argp (unless `--disable-argp`, which falls back to `getopt` for `rlcli` and
   `cli`.

Some dependencies are not necessary if certain outputs are not being built.
The options to disable various outputs for configure.py are:

 - `--disable-server`
 - `--disable-client=CLIENT`
 - `--disable-test-tool=TOOL`
 - `--disable-tool=TOOL`

## W64

Use `./configure.py --build=w64`. You may want `--disable-readline` too, and
building has only been tested with the `luajit` backend.

### ArchLinux instructions

Install the `mingw-w64` group.

AUR packages needed:

 - `mingw-w64-environment`
 - `mingw-w64-pkg-config`
 - `mingw-w64-configure`
 - `mingw-w64-pdcurses`
 - `mingw-w64-termcap`
 - `mingw-w64-readline`
 - `mingw-w64-zlib`
 - `mingw-w64-openssl`
 - `mingw-w64-libevent`
 - `mingw-w64-luajit`
 - `mingw-w64-sqlite`

To build without readline, pass `--disable-readline` to `configure.py`. Note
that even with readline, the `rlcli` tool does not work well with cmd.exe or
powershell, and that `mingw-w64-sqlite` depends on it regardless.

The build is static. DLLs for libevent, luajit, sqlite, winpthreads, etc. don't
need to be distributed with the executables.

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
 - [ ] Add a basic website
