#!/bin/bash

#for i in {1..15} ; do
#    echo "$i"
#    echo -e "foo\nbar"
#    sleep 2
#done | netcat localhost 10101 -c

netcat localhost 10101 -c
