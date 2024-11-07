#!/usr/bin/python

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

        result = subprocess.run("ninja")
        fileresults += [(auditline, result.returncode)]

    with open(filename, 'w') as f:
        for line in lines:
            f.write(line)

    results += [(filename, fileresults)]
    print("AUDIT", filename, "END")

print("AUDIT RESULTS")
for filename, fileresults in results:
    for line, code in fileresults:
        print("AUDIT", filename, "INCLUDE", line[:-1], "CODE", code)
