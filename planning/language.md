# Plan for Command Language and Syntax

## Lexical

Commands are composed of a series of one or more particles terminated by a
newline.

Particles come in the following forms:

 - Keywords like `SAY` or `LOOK`, which are composed of `[a-z]` and `-+*/%!`
   for their first character and `[a-z]`, `[0-9]`, and `-+*/%!` for any further
   characters.

 - IDs which are composed of `[0-9]`

 - Names which begin and end with `"` are composed of printable ASCII
   characters other than `"` as well as spaces.

 - Nested commands which begin with the `(` particle and end with the `)`
   particle and can contain further nested commands.

Example:

`PLAY CARD "Small Cheese" FACE DOWN IN ZONE 4`

lexes to: Keyword `PLAY`, Keyword `Card`, Name `Small Cheese`, Keyword `Face`,
Keyword `DOWN`, Keyword `IN`, Keyword `ZONE`, ID `4`.

Example:

`PLAY CARD (LOOK ID HAND POSITION 1) FACE DOWN IN ZONE 4`

lexes to: Keyword `PLAY`, Keyword `Card`, Begin Nested Command `(`, Keyword `LOOK`,
Keyword `ID`, Keyword `HAND`, Keyword `POSITION`, ID `1`, End Nested Command `)`,
Keyword `Face`, Keyword `DOWN`, Keyword `IN`, Keyword `ZONE`, ID `4`.

Example:

`ZONE7` is a keyword

`7ZONE` is invalid

`(PLAY` is a Begin Nested Command and a Keyword

`DOWN)` is a keyword and an End Nested Command

`CARD"` is invalid

`PLAY(` is invalid

Lines that end before all nested commands and names are terminated are invalid.


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

### Operators

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

 - `ZONE <id>` or `ZONE <name>`, a specific zone

 - `GRAVE`, the grave pile

#### `cardspec`

One of:

 - `CARD <id>`

 - `CARD "<name>"`

 - `CARD <location>`

Card names are contained in quotes. Card names can contain space (` `) and any
printable character except double quote (`"`).

Examples:

```
LOOK HAND

    POSITION ID     CARD
    1        102    Big Bucket
    2        69     Corn Monster
    3        115    Big Bucket

PLAY CARD FROM HAND POSITION 1
PLAY CARD ID 102
PLAY CARD "Big Bucket"


LOOK ZONE 1 (LOOK ZONE ID 1?)

    POSITION    ID  CARD        MODE    FACE    Status
    1           32  Blue Goo    Attack  Up      (none)
    2           34  Blue Goo    Defense Up      (none)

DESTROY CARD ZONE 1 POSITION 1

    Destroyed card ID 32

DESTROY CARD ZONE 1 "Blue Goo"

    Ambiguous: could refer to card ID 32 or 34.

```

#### `LIST`

`LIST PLAYERS`

List every player, their id, and their name.

`LIST ZONES playerspec`

Lists the zones and their ids and names for the specified player.


#### `LOOK`

`LOOK ENERGY`

Show the current play-energy.

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

`LOOK cardspec`

Shows the skills list, equipment, and status effects on the card

`LOOK playerspec stackspec`

As `LOOK stackspec` but only for stacks owned by the specified player.


#### `FIND`

`FIND cardspec`

Attempts to find a card.

`FIND CARD <id>`

Returns the location of the card with this id if there is a card with that id
in any stack.

`FIND CARD <name>`

Returns the location of every card and its ID visible to you with that name,
or none.

`FIND cardspec stackspec`

Narrows the search of a `FIND CARD cardspec` to a specific stack.

`FIND stackspec`

Returns whether or not a stackspec exists and information on where it does if so

#### `LOOKUP`

`LOOKUP cardspec`

Looks up the rules associated with a card.

`LOOKUP CARD <id>`

Looks up the rules for this card, if the card is visible to you.

`LOOKUP CARD <name>`

Looks up the rules for a card with this name, returning an error if no such
card exists.

### Commands During Your Turn

#### `face`

 - `up`
 - `down`

#### `DRAW`

`DRAW`

Draws a card from the deck

#### `MOVE`

#### `PLAY`

`PLAY cardspec ZONE face`

Adds the card into play either face up or down

`ACTIVATE cardspec face`