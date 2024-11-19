# `save_create`

Given a JSON file describing a save and a list of card bundles, create a fresh
`.card_save` file.

Syntax: `save_create OUTPUT_NAME JSON_FILE [BUNDLES...]`

## The JSON File

At the top level, an object with the following fields:

 - `rules`
 - `players`
 - `metadata`
 - `log`
 - `cards`

The `rules` field contains an array, where each element is an object with a
`key` and a `value` field that each contain a string.

The `players` field contains an array, where each element is an object with
an `id` and a `name` field. `id` is a numeric value, `name` is a string.

The `metadata` table is laid out the same as the `rules` table.

The `log` field contains an array where each element is an object with a
`player_id` field (corresponding to an entry in the `players` table with that
`id`) and a `command` field with a string value.

The `cards` field contains an array where each element is an object, each with
a `filename` field (a string equal to the corresponding `filename` property of
the bundle they came from) and a `checksum` field calculated from the script in
the bundle they came from)

## Validation

The `save_create` tool shall indicate an error in the following cases:

 - `OUTPUT_NAME` cannot be opened as a SQLite database for writing

 - `JSON_FILE` is not valid JSON

 - The structure and type of the JSON file does not match the above description

 - Any `BUNDLE`s cannot be opened as a SQLite database for reading, or do
   not exist

 - Any `id` fields in the players table are not unique

 - Any `player_id` fields in the log table do not have a corresponding entry
   in the players table

 - There are cards in the cards table that do not match any of the cards in
   the listed bundles, either because there are no matching filenames or
   because for any matching filenames, the checksum does not match

 - Any errors are encountered using the SQLite API to create the output save

