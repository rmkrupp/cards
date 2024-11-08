#!/bin/bash
# File: misc/make_lex_test.sh
# Part of cards <github.com/rmkrupp/cards>
#
# Copyright (C) 2024 Noah Santer <n.ed.santer@gmail.com>
# Copyright (C) 2024 Rebecca Krupp <beka.krupp@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

mkdir -p test.gen

echo "Making 3 million test cases (this takes a while...)"

echo "First, with 0 to 16 particles of length 0-16 [1000000-16x16]"

./generate_lex_input.py -n 1000000 -c 16 -p 16 --percent > test.gen/1000000-16x16

echo "Then, with 0 to 16 particles of length 0-64... [1000000-64x16]"

./generate_lex_input.py -n 1000000 -c 16 -p 64 --percent > test.gen/1000000-64x16

echo "Then, with 0 to 128 particles of length 0-64... [1000000-64x128]"

./generate_lex_input.py -n 1000000 -c 128 -p 64 --percent > test.gen/1000000-64x128

echo "Done."
