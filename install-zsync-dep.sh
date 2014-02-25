#!/bin/bash
set -e
set -o

CONFIGURE_HOST_32="i686-linux-gnu"
CONFIGURE_FLAG_32="-m32"

# ZeroSync 64-bit install

mkdir build
cd build

#   Build required ZeroSync projects first
sudo apt-get install g++ git automake autoconf

#   libsodium
git clone git://github.com/jedisct1/libsodium.git
cd libsodium
./autogen.sh
if [[ $HOST == "32" ]]; then
    ./configure --host=$CONFIGURE_HOST_32 "CFLAGS=$CONFIGURE_FLAG_32" "CXXFLAGS=$CONFIGURE_FLAG_32" "LDFLAGS=$CONFIGURE_FLAG_32"
else
    ./configure
fi
make check
sudo make install # libdir=absolutePath includedir=absolutePath
sudo ldconfig
cd ..

#   libzmq
git clone git://github.com/zeromq/libzmq.git
cd libzmq
./autogen.sh
if [[ $HOST == "32" ]]; then
    ./configure --host=$CONFIGURE_HOST_32 "CFLAGS=$CONFIGURE_FLAG_32" "CXXFLAGS=$CONFIGURE_FLAG_32" "LDFLAGS=$CONFIGURE_FLAG_32"
else
    ./configure
fi
make check
sudo make install
sudo ldconfig
cd ..

#   CZMQ
git clone git://github.com/zeromq/czmq.git
cd czmq
./autogen.sh
if [[ $HOST == "32" ]]; then
    ./configure --host=$CONFIGURE_HOST_32 "CFLAGS=$CONFIGURE_FLAG_32" "CXXFLAGS=$CONFIGURE_FLAG_32" "LDFLAGS=$CONFIGURE_FLAG_32"
else
    ./configure
fi
make check
sudo make install
sudo ldconfig
cd ..

#   Zyre
git clone git://github.com/zeromq/zyre.git
cd zyre
./autogen.sh
if [[ $HOST == "32" ]]; then
    ./configure --host=$CONFIGURE_HOST_32 "CFLAGS=$CONFIGURE_FLAG_32" "CXXFLAGS=$CONFIGURE_FLAG_32" "LDFLAGS=$CONFIGURE_FLAG_32"
else
    ./configure
fi
make check
sudo make install
sudo ldconfig
cd ..

#   Build and check libzsync
script: ./autogen.sh && ./configure && make # && make check

