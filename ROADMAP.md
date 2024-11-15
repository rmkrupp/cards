# Roadmap to 1.0

## Version 0.1

### Planning and Data

 - [ ] sketch out a document with example API for card scripts

 - [ ] make example scripts for every type of card

### Server

 - [ ] load cards from bundles

 - [ ] add another category of name: subtypes

 - [x] normalize card, ability, and subtype names when loaded

 - [x] reentrant lexer

 - [x] make sure the lexer is UTF-8 transparent, at least for names and
       possibly for keywords

 - [x] normalize inputted names for matching against cards/abilities

   - [x] UTF-8 aware casefolding

 - [x] determine if cards should have a Display Name, and if the non-display
       name should be specified explicitly or created automatically from the
       display, or automatic with optional override

 - [ ] set up types for commands and syntax trees (nested commands)

 - [ ] set up the parser to return command objects after parsing

 - [ ] set up games (or the parser?) to have an interface to be fed particle
       buffers (the result of lexing) (or command strings?)

 - [ ] load commands from save game files and feed them all to a parser

   - [ ] load card bundles from save game

 - [ ] set up player types within the game object

   - [ ] extract player lists from save games

 - [ ] basic feeding from networker objects to parser objects

 - [ ] maintain a log of every command sent to the parser

   - [ ] transform particles back into commands?

   - [ ] prepare for this to be filtered and simplified to keep only those
         commands which result in a game state change

   - [ ] implement state tracking (i.e. a counter) in the game object and flag
         state-changing commands as they come back from the parser

   - [ ] save logs, bundles, and metadata to save game files

### Tools

 - [ ] save game file inspector tool (extract metadata, player lists, and log)

 - [ ] save game creator tool (given a text log etc. create a save game)

## Version 0.2

### Server

 - [ ] parse subset of command grammar

 - [ ] add functionality to run commands and modify state

   - [ ] SAY command

   - [ ] EXIT command, SHUTDOWN command

   - [ ] AGREE command, READY command

   - [ ] commands to inspect rules, look up cards, and choose a deck

   - [ ] "suggested" deck functionality

   - [ ] commands to choose a player name

   - [ ] commands to end a game /  leave

 - [ ] observer vs active players

   - [ ] handle connections that occur after the game has started

   - [ ] ability to downgrade from active to observer ("give up")

   - [ ] ability to pause the game in some way when a player leaves and then
         allow them to rejoin when they reconnect

 - [ ] represent game state:

   - [ ] stacks that hold cards in an order

   - [ ] zones where cards are played

   - [ ] cards in zones and stacks, with modes and faces

   - [ ] cards attributes (aa/ad/dd, subtype)

   - [ ] equipment attached to cards

   - [ ] status effects active on cards

   - [ ] life

   - [ ] energy

   - [ ] turn

## Version 0.3

### Planning and Data

 - [ ] robust selection of cards, designed to test and require a wide array of
       functionality vs. being a good game

### Server

 - [ ] add functionality for providing game-state introspection and
       manipulation to the card scripts
 
 - [ ] turn phases, active player, and player turn order

 - [ ] basic gameplay

   - [ ] functions to move cards from stacks to other stacks, including
         drawing and discarding, and from one position in a stack to another

   - [ ] attributes of stacks to control permissions and what functions can
         be performed

   - [ ] functions to move cards from stacks to zones and visa versa

   - [ ] functions to alter face and mode of cards in zones

   - [ ] functions to select an ability from a card and invoke it

   - [ ] functions to alter life and energy, and test them as a prerequisite
         for other actions

   - [ ] functions to select and find zero or more cards by parameters and to
         determine when parameters are unambiguous

   - [ ] functions to modify turn, turn order, etc.

## Version 0.4

### Planning and Data

 - [ ] a list of desired rules and how they affect gameplay

### Server

 - [ ] parse and execute the core functionality of a turn

   - [ ] PLAY, MOVE, whatever we call changing face and mode

   - [ ] DISCARD

   - [ ] inspect game state with LOOK

   - [ ] get LOOK targets with LIST

   - [ ] ATTACK with a character

   - [ ] ACTIVE an ability

   - [ ] EQUIP an equipment, and UNEQUIP

   - [ ] END TURN

   - end game conditions

 - [ ] utility commands for nested commands that introspect the game state in
       a scripting-friendly way

 - [ ] nested commands

 - [ ] some simple scripted functionality (e.g. abilities) should function and
       affect the game state, both ongoing (removable) and instant

 - [ ] commands to load save games and create save games

 - [ ] commands to inspect and change rules in the lobby, and functionality
       derived from those rules (e.g. starting life, max hand size, etc.)

## Version 0.5+

### Planning and Data

 - [ ] brainstorm the "default" game theme

 - [ ] brainstorm cards and decks for the default theme

 - [ ] playtest those decks

### Server

 - [ ] triggers

 - [ ] fully-featured commands for nesting and scripting (e.g. trigger scripts)

 - [ ] fully-featured API within Lua for card behavior, including automatically
       activated (i.e. passive) abilities as well as ones requested with
       ACTIVATE, equipment effects, and status effects

### Client

 - [ ] one of:

   - [ ] a browser-based (websocket) client with a server, implemented in
         node.js

   - [ ] a curses-based text client, cross platform and with UTF-8 support

 - [ ] computer-controlled client

## Version 1.0

 - [ ] robust default gameplay (cards, rules, visuals, music, etc.)

 - [ ] graphical (3D) client

   - [ ] fetch and manifest assets (audio and visual) from server

   - [ ] animate changes to game state

 - [ ] packages for ArchLinux and Debian

 - [ ] smooth Windows installation/deployment

   - [ ] facility to launch sub-packages (e.g. server, computer-controlled)
         from graphical client on windows

 - [ ] rigorous testing of server behavior under adverse conditions

 - [ ] basic website

 - [ ] computer-controlled client(s)

 - [ ] investigate steam SDK integration
