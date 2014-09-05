#!/bin/sh

if [ "$1" = "" ]; then
  echo "Specify directory."
  exit 1
fi

L=`find $1 | sort | grep 'z...[.]bz2'`

for i in $L ; do
  NM=`echo $i | awk '{print substr($0,length($0)-7);}'`
  echo $NM
done
