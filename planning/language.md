# Plan for Command Language and Syntax

## Smart Client Commands

`@SOURCE <filename>`

The client runs every command in `filename` as if it was typed in.

## Always Available Commands

`SAY [something]`

Sends a text message to all players

`VERSION`

Get the version from the server

`RULES`

Get the configurable rules from the server

## Admin Commands

???

## Commands In The Lobby

`READY`

Become ready for the game to start. Once all players are ready, the game
starts. You cannot become un-ready.

`SHOW DECKS`

Show all decks the server knows about, listing them in the form:

```
ID ALIAS        #CARDS  USING
0  "Blue Deck"  60      P0, P1
1  "Red Deck"   15
```

`SHOW DECK <id>` or `SHOW DECK <name>`

Show the cards in the deck matching that ID or name, or respond with failure
if no such deck exists. Respond in the form:

```
ID CARD
0  Blue Ranger
1  Blue Ranger
2  Big Tree
3  Magic Acorn
4  Mud Monster
(etc.)
```

`USE DECK <id>` or `USE DECK <name>`

Change which deck you are using to the deck matching that ID or name.

#### Note

Potentially, an interface for dynamically creating decks (leveraging the ability
of clients to source a script for that deck) could be added. This would probably
necessitate having an "owner" of decks, letting players copy or clone decks
to become owners, adding commands to remove/add etc., and potentially an
approval round after (or before?) "ready" where players can veto other players
decks.

## Commands in Game

### Commands At Any Time

#### `playerspec`

One of:

 - `MY`, meaning the invoking player

 - `PLAYER <id>`, meaning the player matching the id

 - `PLAYER <name>`, meaning the player matching the name

#### `stackspec`

One of:

 - `HAND`, the hand

 - `DECK`, the deck

 - `DISCARD`, the discard pile

 - `GRAVE`, the grave pile

#### `LIST`

`LIST PLAYERS`

List every player, their id, and their name.

#### `LOOK`

`LOOK ENERGY`

Show the current energy.

`LOOK ENERGY SOURCES`

List the current sources of energy.

`LOOK playerspec ENERGY`

Show the amount of energy contributed by a player's sources.

`LOOK playerspec ENERGY SOURCES`

List the current sources of energy owned by a player.

`LOOK LIFE`

List the current life of every player

`LOOK playerspec LIFE`

Show the life of a player.

`LOOK stackspec`

Show the known information about every of one kind of stack. This could be only
its existance, the size of the stack, or the contents of the stack.

`LOOK playerspec stackspec`

As `LOOK stackspec` but only for stacks owned by the specified player.





### Commands During Your Turn

