#!/bin/sh

BNAME=2masskit

if [ "$1" = "" ]; then
  echo "Specify version"
else
  PNAME="$BNAME-$1"
  rm -rf $PNAME
  cp -dpuR ../db/pgsql $PNAME
  cd $PNAME
  make -f Makefile.linux64 clean
  L=`find . | grep '~$'`
  if [ "$L" != "" ]; then
    rm $L
  fi
  L=`find . | grep 'old$'`
  if [ "$L" != "" ]; then
    rm -r $L
  fi
  L=`find . | grep 'old2$'`
  if [ "$L" != "" ]; then
    rm -r $L
  fi
  L=`find . | grep 'bak$'`
  if [ "$L" != "" ]; then
    rm -r $L
  fi
  cd ..
  tar zcf $PNAME.tar.gz $PNAME
  rm -rf $PNAME
fi
