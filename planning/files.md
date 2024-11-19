# File Structures

## Card Bundles

Bundles, which have by convention the `.bundle` extension, are SQLite
databases.

They have the following schema:

 - `CREATE TABLE cards (filename, script);`

The filename column holds the original filenames to the scripts passed when
the bundle was created and is used to display error messages in the event that
the name of the card cannot be extracted from the script and used instead.

The script column holds the Lua script.

We may add a metadata table to bundles in the future giving bundles details
like a name and author.

We may add data in the future to support clients that wish to include images
for the cards as well.

## Saves

Saves, which have by convention the `.cards_save` extension, are SQLite
databases.

They have the following schema:

 - `CREATE TABLE metadata (key, value);`

The key and value columns are both textual and hold key/value pairs of
metadata. (See below.)

 - `CREATE TABLE cards (filename, script);`

A bundle holding every card in use by the save. `filename` is a text field and
`script` is a blob field.

 - `CREATE TABLE players (id, name);`

A list of every player by `name` (text) and that player's numeric `id`.

 - `CREATE TABLE log (player_id, command);`

A list of every command run that modified the game state (i.e. the commands
that need to be run to recreate the game) and what player ran them. `command`
is a text field, `player_id` is numeric and matches the id in players.

 - `CREATE TABLE rules (key, value);`

A list of every rule needed to recreate the game state with the command log.
`key` and `value` are both textual.
