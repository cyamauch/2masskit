#!/bin/sh

if [ "$1" = "" ]; then
  echo "Specify directory."
  exit 1
fi

L0=`find $1 | sort -r | grep 's...[.]dat[.]gz'`
L1=`find $1 | sort | grep 'n...[.]dat[.]gz'`

for i in $L0 $L1 ; do
  NM=`echo $i | awk '{print substr($0,length($0)-10);}'`
  echo $NM
done
