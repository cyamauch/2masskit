#!/bin/sh

if [ "$1" = "" ]; then
  echo "Specify directory."
  exit 1
fi

L=`find $1 | sort | grep '[.]cat'`

for i in $L ; do
  NM=`echo $i | awk '{print substr($0,length($0)-12);}'`
  SZ=`ls -l $i | awk '{print $5}'`
  echo $NM $SZ
done
