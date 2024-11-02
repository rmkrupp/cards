#!/usr/bin/python

import random

words = set()
with open("/usr/share/dict/words") as f:
    for line in f:
        words.add(line.lower()[:-1])

words = list(words)

for n in range(1000):
    print(' '.join(random.choice(words) for k in range(random.choice([1,2,3,4]))))
