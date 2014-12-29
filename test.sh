#!/bin/sh
UNAME=`uname -s`
if   [ ${UNAME} == "Darwin" ]; then
    APP=bin/hgr2rgb.osx
elif [ ${UNAME} == "Linux" ]; then
    APP=bin/hgr2rgb.elf
fi

${APP} -tga hgr/archon.hgr2
${APP} -bmp hgr/archon.hgr2

