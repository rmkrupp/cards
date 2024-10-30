#!/usr/bin/python3

from random import randint, choice
import string
import argparse
import sys

parser = argparse.ArgumentParser(
        prog="generate_lex_input",
        description="generate (valid) nonsense for the lexer to lex"
    )

parser.add_argument(
        "-p", "--particle-length",
        type=int,
        default=64,
        help="set the maximum length of keyword, name, and number particles"
    )
parser.add_argument(
        "--particle-length-min",
        type=int,
        default=0,
        help="set the minimum length of keyword, name, and number particles"
    )
parser.add_argument(
        "-c", "--command-length",
        type=int,
        default=128,
        help="set the maximum length of commands (in number of particles)"
    )
parser.add_argument(
        "--command-length-min",
        type=int,
        default=1,
        help="set the minimum length of commands (in number of particles)"
    )
parser.add_argument(
        "-n", "--number",
        type=int,
        default=1000,
        help="set the number of commands to generate"
    )
parser.add_argument(
        "--keyword-weight",
        type=int,
        default=1,
        help="set the relative frequency of keyword particles"
    )
parser.add_argument(
        "--number-weight",
        type=int,
        default=1,
        help="set the relative frequency of number particles"
    )
parser.add_argument(
        "--name-weight",
        type=int,
        default=1,
        help="set the relative frequency of name particles"
    )
parser.add_argument(
        "--begin-nest-weight",
        type=int,
        default=1,
        help="set the relative frequency of begin-nest particles"
    )
parser.add_argument(
        "--end-nest-weight",
        type=int,
        default=1,
        help="set the relative frequency of end-nest particles"
    )
parser.add_argument(
        "--percent",
        action="store_true",
        help="print percentage completion to stderr"
    )

args = parser.parse_args()

name_forbidden = "\n\"\r\x0b\x0c"
particle_name_letters = [c for c in string.printable if c not in name_forbidden]
particle_num_letters = [c for c in string.digits]
particle_keyword_first_letters = [c for c in string.ascii_letters + "+-*/?!"]
particle_keyword_letters = [c for c in string.ascii_letters + string.digits + "!?-"]

def particle_name():
    s = "\""
    for i in range(randint(args.particle_length_min, args.particle_length)):
        s += choice(particle_name_letters)
    s += "\""
    return s

def particle_num():
    s = ""
    for i in range(randint(max(1, args.particle_length_min), args.particle_length)):
        s += choice(particle_num_letters)
    return s

def particle_begin_nest():
    return '('

def particle_end_nest():
    return ')'

def particle_keyword():
    s = choice(particle_keyword_first_letters)
    for i in range(randint(args.particle_length_min, args.particle_length - 1)):
        s += choice(particle_keyword_letters)
    return s

choices = []
choices += [particle_keyword] * args.keyword_weight
choices += [particle_name] * args.name_weight
choices += [particle_num] * args.number_weight
choices += [particle_begin_nest] * args.begin_nest_weight
choices += [particle_end_nest] * args.end_nest_weight

pct = 0
for i in range(args.number):
    if args.percent and int((100 * i) / args.number) > pct:
        pct = int((100 * i) / args.number)
        print(str(pct) +"%", file=sys.stderr)

    c = ""
    for i in range(randint(args.command_length_min, args.command_length)):
        f = choice(choices)
        c += f() + " "
    print(c)

