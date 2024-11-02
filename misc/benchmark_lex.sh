#!/bin/bash

function benchmark() {
    echo "$1"
    time ../test/lex_test --silent < test.gen/"$1"
}

benchmark 1000000-16x16
benchmark 1000000-64x16
benchmark 1000000-64x128
