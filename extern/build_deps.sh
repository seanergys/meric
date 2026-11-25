#!/bin/bash

SCRIPT_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
EXTERN_ROOT=$SCRIPT_ROOT

pushd $EXTERN_ROOT

if [ ! -d "$EXTERN_ROOT/capnproto-1.5.0.src" ]; then
    wget https://github.com/capnproto/capnproto/archive/refs/tags/v1.5.0.tar.gz
    tar -xvzf v1.5.0.tar.gz
    mv capnproto-1.5.0 capnproto-1.5.0.src
fi

if [ ! -d "$EXTERN_ROOT/seanerbus-cpp.src" ]; then
    git clone --recurse-submodules git@gitlab.seanergys.fz-juelich.de:software/seanerbus-cpp.git seanerbus-cpp.src
fi

if [ ! -d "$EXTERN_ROOT/capnproto" ]; then
    pushd capnproto-1.5.0.src/c++
    make clean
    autoreconf -i
    ./configure --prefix=$EXTERN_ROOT/capnproto
    make -j4
    make install
    popd
fi

if [ ! -d "$EXTERN_ROOT/seanerbus-cpp" ]; then
    export PATH=$PATH:$EXTERN_ROOT/capnproto/bin
    export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$EXTERN_ROOT/capnproto/lib64/pkgconfig
    cmake -S seanerbus-cpp.src -B seanerbus-cpp.src/build -DCMAKE_INSTALL_PREFIX=$EXTERN_ROOT/seanerbus-cpp
    cmake --build seanerbus-cpp.src/build
    cmake --install seanerbus-cpp.src/build --prefix $EXTERN_ROOT/seanerbus-cpp
fi

popd