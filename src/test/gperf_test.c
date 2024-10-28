#include <stdio.h>
#include <string.h>

#include "command/keyword.h"

int main(int argc, char ** argv)
{
    for (int i = 1; i < argc; i++) {
        if (keyword_lookup(argv[i], strlen(argv[i]))) {
            printf("%s: yes\n", argv[i]);
        } else {
            printf("%s: no\n", argv[i]);
        }
    }
}
