# Test Tools

## `lex_test`

By default, this tool runs each line through the lexer and prints a
representation of the tokens, or an error if one occurs.

It matches any keyword token against the known keywords, too. Known keywords
are printed as `KEYWORD<...>` while unknown ones are printed as
`KEYWORD*<...>`.

It supports the following options which may be passed in any position, with
latter options superseding earlier ones:

 - `--silent`: only print a single line, at the end of all lexing, with the
   number of errors and the total lines lexed

 - `--errors`: like silent, but print any lines (without any changes or
   messages where the lexer returns an error.)

If neither `--silent` or `--errors` are given, this tool detects whether its
output is going to a terminal and prints the error message slightly different
depending (including by printing an offset carrot to match the input line if
it detects a terminal.)

## `gperf_test`

This tool looks up every argument passed in the keyword list and prints a
message indicating whether the lookup succeeded.

## `hash_test`

Performs a very rudimentary test of the the hashing library. See the test tools
that are part of that library for more complex tests or benchmarking.

## `sorted_set_test`

Adds some random strings to a sorted set and then dumps it in GraphViz (i.e.
.dot) format.

