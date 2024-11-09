#!/usr/bin/python
# File: misc/audit-headers.py
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

import sys, subprocess

results = []

print("AUDIT", " ".join(sys.argv[1:]))

for filename in sys.argv[1:]:
    print("AUDIT", filename, "BEGIN")

    includes = []
    fileresults = []
    with open(filename) as f:
        lines = f.readlines()

    for line in lines:
        if len(line) > 8 and line[:8] == "#include":
            includes += [line]

    print(includes)

    for auditline in includes:
        print("AUDIT", filename, "REMOVING", auditline[:-1])
        with open(filename, 'w') as f:
            for line in lines:
                if line == auditline:
                    pass
                else:
                    f.write(line)

        result = subprocess.run(["ninja", "--quiet"])
        fileresults += [(auditline, result.returncode)]

    with open(filename, 'w') as f:
        for line in lines:
            f.write(line)

    results += [(filename, fileresults)]
    print("AUDIT", filename, "END")

print("AUDIT RESULTS")
for filename, fileresults in results:
    for line, code in fileresults:
        if len(sys.argv) == 2 or code == 0:
            print("AUDIT", filename, "INCLUDE", line[:-1], "CODE", code)
