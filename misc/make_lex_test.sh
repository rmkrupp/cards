#!/bin/bash

mkdir -p test.gen

echo "Making 3 million test cases (this takes a while...)"

echo "First, with 0 to 16 particles of length 0-16 [1000000-16x16]"

./generate_lex_input.py -n 1000000 -c 16 -p 16 --percent > test.gen/1000000-16x16

echo "Then, with 0 to 16 particles of length 0-64... [1000000-64x16]"

./generate_lex_input.py -n 1000000 -c 16 -p 64 --percent > test.gen/1000000-64x16

echo "Then, with 0 to 128 particles of length 0-64... [1000000-64x128]"

./generate_lex_input.py -n 1000000 -c 128 -p 64 --percent > test.gen/1000000-64x128

echo "Done."
