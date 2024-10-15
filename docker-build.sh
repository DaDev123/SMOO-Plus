#!/bin/bash
set -euo pipefail

ISEMU=${1:-0}
if [ "$ISEMU" -ne "1" ] ; then
  ISEMU="0"
fi

# clean up build artifacts when switching to/from emulator builds
if [ "$ISEMU" -eq "1" ] && ! [ -f ./build100/.for_emulators ] ; then
  rm  -rf  ./build100/
elif [ "$ISEMU" -eq "0" ] && ! [ -f ./build100/.for_switch ] ; then
  rm  -rf  ./build100/
fi

# remember what system this build is for
mkdir  -p  ./build100/
if [ "$ISEMU" -eq "1" ] ; then
  touch  ./build100/.for_emulators
else
  touch  ./build100/.for_switch
fi

# build
export DOCKER_BUILDKIT=1
docker  build  .  -t smoo-client-build
docker  run  --rm       \
  -u $(id -u):$(id -g)  \
  -v "/$PWD/":/app/     \
  -e ISEMU=${ISEMU}     \
  smoo-client-build     \
;
docker  rmi  smoo-client-build

# copy romfs
DIR=$(dirname ./starlight_patch_*/atmosphere/)
cp  -r  ./romfs/  $DIR/atmosphere/contents/0100000000010000/.

# create file structure for emulator builds
if [ "$ISEMU" -eq "1" ] ; then
  rm  -rf  $DIR/SMOO/
  mkdir  -p  $DIR/SMOO/
  mv  $DIR/atmosphere/contents/0100000000010000/exefs  $DIR/SMOO/exefs
  mv  $DIR/atmosphere/contents/0100000000010000/romfs  $DIR/SMOO/romfs
  mv  $DIR/atmosphere/exefs_patches/StarlightBase/3CA12DFAAF9C82DA064D1698DF79CDA1.ips  $DIR/SMOO/exefs/
  rm  -rf  $DIR/atmosphere/
fi
