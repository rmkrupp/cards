find . -name '.git' -prune -o -name 'pkg' -prune -o  -name '*.[ch]'  -type f -print -execdir grep TODO {} \;
