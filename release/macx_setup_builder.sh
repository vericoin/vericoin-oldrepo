#!/bin/sh

# Note: The structure of this package depends on the -rpath,./lib to be set at compile/link time.

version="1.6.5.5"
arch=`uname -m`

if [ "${arch}" = "x86_64" ]; then
    arch="64bit"
else
    arch="32bit"
fi

if [ -f RoxyCoin-Qt.app/Contents/MacOS/RoxyCoin-Qt ] && [ -f roxycoin.conf ] && [ -f README ]; then
    echo "Building RoxyCoin_${version}_${arch}.pkg ...\n"
    cp roxycoin.conf RoxyCoin-Qt.app/Contents/MacOS/
    cp README RoxyCoin-Qt.app/Contents/MacOS/

    # Remove the old archive
    if [ -f RoxyCoin_${version}_${arch}.pkg ]; then
        rm -f RoxyCoin_${version}_${arch}.pkg
    fi

    # Deploy the app, create the plist, then build the package.
    macdeployqt ./RoxyCoin-Qt.app -always-overwrite
    pkgbuild --analyze --root ./RoxyCoin-Qt.app share/qt/RoxyCoin-Qt.plist
    pkgbuild --root ./RoxyCoin-Qt.app --component-plist share/qt/RoxyCoin-Qt.plist --identifier org.roxycoin.RoxyCoin-Qt --install-location /Applications/RoxyCoin-Qt.app RoxyCoin_${version}_${arch}.pkg
    echo "Package created in: $PWD/RoxyCoin_${version}_${arch}.pkg\n"
else
    echo "Error: Missing files!\n"
    echo "Run this script from the folder containing RoxyCoin-Qt.app, roxycoin.conf and README.\n"
fi

