#!/bin/sh

# This script depends on the GNU script makeself.sh found at: http://megastep.org/makeself/
# Note: The structure of this package depends on the -rpath,./lib to be set at compile/link time.

version="1.7"
arch=`uname -i`

if [ "${arch}" = "x86_64" ]; then
    arch="64bit"
    QtLIBPATH="${HOME}/Qt5.4.2/5.4/gcc_64"
else
    arch="32bit"
    QtLIBPATH="${HOME}/Qt5.4.2/5.4/gcc"
fi

if [ -f roxycoin-qt ] && [ -f roxycoin.conf ] && [ -f README ]; then
    echo "Building RoxyCoin_${version}_${arch}.run ...\n"
    if [ -d RoxyCoin_${version}_${arch} ]; then
        rm -fr RoxyCoin_${version}_${arch}/
    fi
    mkdir RoxyCoin_${version}_${arch}
    mkdir RoxyCoin_${version}_${arch}/libs
    mkdir RoxyCoin_${version}_${arch}/platforms
    mkdir RoxyCoin_${version}_${arch}/imageformats
    cp roxycoin-qt RoxyCoin_${version}_${arch}/
    cp roxycoin.conf RoxyCoin_${version}_${arch}/
    cp README RoxyCoin_${version}_${arch}/
    ldd roxycoin-qt | grep libssl | awk '{ printf("%s\0", $3); }' | xargs -0 -I{} cp {} RoxyCoin_${version}_${arch}/libs/
    ldd roxycoin-qt | grep libdb_cxx | awk '{ printf("%s\0", $3); }' | xargs -0 -I{} cp {} RoxyCoin_${version}_${arch}/libs/
    ldd roxycoin-qt | grep libboost_system | awk '{ printf("%s\0", $3); }' | xargs -0 -I{} cp {} RoxyCoin_${version}_${arch}/libs/
    ldd roxycoin-qt | grep libboost_filesystem | awk '{ printf("%s\0", $3); }' | xargs -0 -I{} cp {} RoxyCoin_${version}_${arch}/libs/
    ldd roxycoin-qt | grep libboost_program_options | awk '{ printf("%s\0", $3); }' | xargs -0 -I{} cp {} RoxyCoin_${version}_${arch}/libs/
    ldd roxycoin-qt | grep libboost_thread | awk '{ printf("%s\0", $3); }' | xargs -0 -I{} cp {} RoxyCoin_${version}_${arch}/libs/
    ldd roxycoin-qt | grep libminiupnpc | awk '{ printf("%s\0", $3); }' | xargs -0 -I{} cp {} RoxyCoin_${version}_${arch}/libs/
    ldd roxycoin-qt | grep libqrencode | awk '{ printf("%s\0", $3); }' | xargs -0 -I{} cp {} RoxyCoin_${version}_${arch}/libs/
    cp ${QtLIBPATH}/lib/libQt*.so.5 RoxyCoin_${version}_${arch}/libs/
    cp ${QtLIBPATH}/lib/libicu*.so.53 RoxyCoin_${version}_${arch}/libs/
    cp ${QtLIBPATH}/plugins/platforms/lib*.so RoxyCoin_${version}_${arch}/platforms/
    cp ${QtLIBPATH}/plugins/imageformats/lib*.so RoxyCoin_${version}_${arch}/imageformats/
    strip RoxyCoin_${version}_${arch}/roxycoin-qt
    echo "Enter your sudo password to change the ownership of the archive: "
    sudo chown -R nobody:nogroup RoxyCoin_${version}_${arch}

    # now build the archive
    if [ -f RoxyCoin_${version}_${arch}.run ]; then
        rm -f RoxyCoin_${version}_${arch}.run
    fi
    makeself.sh --notemp RoxyCoin_${version}_${arch} RoxyCoin_${version}_${arch}.run "\nCopyright (c) 2014-2015 The RoxyCoin Developers\nRoxyCoin will start when the installation is complete...\n" ./roxycoin-qt \&
    sudo rm -fr RoxyCoin_${version}_${arch}/
    echo "Package created in: $PWD/RoxyCoin_${version}_${arch}.run\n"
else
    echo "Error: Missing files!\n"
    echo "Copy this file to a setup folder along with roxycoin-qt, roxycoin.conf and README.\n"
fi

