# Server Plan

## Overview

The server launches with a list of zero or more config files (written in Lua)
that it reads to determine its configuration.

It then creates a listener (contained in a `struct server`) and enters Lobby
Mode to wait for connections. When connections arrive, it gives each a `struct
connection` and registers them in the `struct server`.

In Lobby Mode, each connection is informed of its ID and what connections have
already arrived. If connections end while in Lobby Mode, their ID is not
recycled. If the server runs out of IDs to give (hitting an internal, hardcoded
limit) it shuts down.

The following commands can affect the server at this point:

 - `shutdown` shuts down the server and causes it to exit
 - `exit` causes the connection to the issuing client to be closed.
 - `ready` causes a connection to enter ready state

Some of these commands may become "admin commands" if such functionality is
introduced.

When all connections are in `ready` state, the server exits Lobby Mode and
enters Game Mode, creating a `struct game` to hold this particular game.
Connections are assigned to players and those players are added to the game.

## The Server

Contains a pointer to the configuration, a `struct networker` reference, and
a `struct game` reference.

The Networker contains the internal, private state of the networking setup.
The server begins the event loop by calling `networker_run()` on this.

The Game contains the state of the game, created when Lobby Mode exits.

Iteration over opaque `struct connection` pointers can be done with the
Networker instance to find the current connections.
