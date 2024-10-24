# Protocol Planning

## Output (Messages from Server -> Clients)

### Option 1a: Only "descriptive"

`Player 2 <extreme_sphere> has played card 17 face down in attack mode in row 2.`

`You have lost 2 life.`

`The total energy is now 8.`

`There are 7 cards in your hand.`

(Side note: is it better for a `LOOK LIFE` command to respond `20` or `Your
life is 20.`? Or are those separate commands? Especialy with regards to
scripting/macros? See Input section.)

#### Pros

 - Server code only has to maintain one authoritative format

 - Minimal text-based clients are extremely simple

 - Logs and replays can use human-readable format by default

 - No server code paths go unused, easing testing

#### Cons

 - Parsing is required, and the description must be crafted to facilitate it,
   potentially harming readability

### Option 1b: Only "descriptive" with a `DESCRIBE IN JSON`

Like 1a but include a command whose response in valid JSON that describes the
whole current state and is easily parsed.

#### Pros

 - Same as 1a

 - Allows more complex clients without requiring them to parse the descriptive
   information by having them call `DESCRIBE IN JSON` every time the state
   changes and parsing that (and optionally comparing to last-known state to
   determine their own delta.)

 - Only the server and clients that use this feature would require a JSON
   dependency

 - The added server JSON usage is concentrated around one specific point

#### Cons

 - Adds a dependency for the server on JSON

 - Clients that want to transform the "descriptive" message e.g. to colorize
   still need to parse it.

 - The base descriptive form may be able to be less easily parsed, though see
   previous

### Option 1c: 1b but there's a "color" mode

Like 1b, but the server can be asked to provide colorized descriptive output.

#### Pros

 - Clients that want color don't have to parse the descriptive text

#### Cons

 - Increased server complexity and the potential to require multiple codepaths
   in the server for each message

 - If colorized is the default, stripping it out is extra logic for a client

### Option 1d: 1c but the server has flexible descriptive backend

`(play (Player 2) (card 17) (row 2) (face down) (mode attack))` or something

#### Pros

 - Could make adding a "marked up" or JSON output easier

#### Cons

 - Is this really any better than native JSON?

 - Even more code paths to test

### Option 2: Only JSON

```
{
    "type": "action",
    "action": "play_card",
    "player": {
        "id": 2,
        "alias": "extreme_sphere"
    },
    "card": 17,
    "row": 2,
    "face": "down",
    "mode": "attack"
}
```

#### Pros

 - Only one code path in the server

#### Cons

 - Simpler clients are now more complicated

 - Code to convert JSON -> description is duplicated if e.g. there's a C client
   and a Javascript based client

### Option 3: JSON with embedded descriptive

```
{
    ...,
    "description": "Player 2 <extreme_sphere> has played card 17 face down in attack mode in row 2."
}
```

#### Pros

 - Clients that mostly want the JSON but also want to display authoritative
   messages from the server can do that

 - Clients may not need to have a parallel code path for making descriptions
   (but see below)

#### Cons

 - If the description isn't exactly what the client wants, the benefit is not
   clear

 - Obvious case of colorizing output still requires parsing, though it could
   be of JSON rather than description (but that would require maintaining two,
   unrelated, codebases for forming descriptions: one in the server and one in
   the client.)

### Option 4: Either 1, 2, or 3, chosen by request

Begin in one of these modes, then let there be a command to select a different
one.

#### Pros

 - If the server provides exactly what the client wants, the client has to do
   less work

 - Might be able to expose some server functionality for building/parsing to
   the client as a library, though only to clients that speak C

#### Cons

 - Server has redundancy and multiple code paths, complicating testing

 - Some of the logic of these modes will be duplicated by clients anyways,
   especially if none of the options are exactly what the client wants

## Input (Player/Client -> Server)

### Option 1: Command Language

`play card 3 face down in attack mode in row 2`

or perhaps,

`play 3 down row 2`

(where 3 is the card's unique ID, learned from something like `LOOK HAND`,
or perhaps `play` default to index in the hand and 3 is the 3rd card, such that
`play card id 17` would refer to card ID 17 which might be the same as the
3rd card in the hand, or even that you could nest like `play card (hand 3)`
where `(hand 3)` evaluates to the ID of the 3rd card in your hand, so on.)

#### Pros

#### Cons

### Option 2: JSON

```

```

#### Pros

#### Cons

### Option 3: Either (negotiated or autodetected)

Either there are commands to switch between formats plus a default, or changes
are auto-detected (e.g. JSON starts with `{`) either for the first command or
for every command.

#### Pros

#### Cons

### Option 4: JSON supports a "raw" command

```
{
    "type": "raw",
    "command": "play card 3 face down in attack mode in row 2"
}
```

#### Pros

#### Cons
