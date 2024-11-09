#!/bin/bash

#
# receive the key needed to validate the AUR packages needed to cross-compile
#

# readline key
gpg --keyserver keyserver.ubuntu.com --recv-keys BB5869F064EA74AB

# openssl key
gpg --keyserver keys.openpgp.org --recv-keys 216094DFD0CB81EF

# libunistring key
gpg --keyserver keyserver.ubuntu.com --recv-keys F5BE8B267C6A406D
