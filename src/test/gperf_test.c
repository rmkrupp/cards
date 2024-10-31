#include <stdio.h>
#include <string.h>

#include "command/keyword.h"

int main(int argc, char ** argv)
{
    for (int i = 1; i < argc; i++) {
        const struct keyword_lookup_result * result;
        if ((result = keyword_lookup(argv[i], strlen(argv[i])))) {
            printf("%s: yes\n", keyword_string(result->offset));
        } else {
            printf("%s: no\n", argv[i]);
        }
    }
}
