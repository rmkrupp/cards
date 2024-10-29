#!/bin/bash

#
# receive the key needed to validate the AUR packages needed to cross-compile
#

# openssl key
gpg --keyserver keys.openpgp.org --recv-keys 216094DFD0CB81EF
