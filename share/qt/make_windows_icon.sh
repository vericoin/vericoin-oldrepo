#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/roxycoin.ico

convert ../../src/qt/res/icons/roxycoin-16.png ../../src/qt/res/icons/roxycoin-32.png ../../src/qt/res/icons/roxycoin-48.png ${ICON_DST}
