#!/bin/sh

if [ "$1" = "" ]; then
  echo "Specify directory."
  exit 1
fi

L=`find $1 | sort | grep 'psc_...[.]gz'`

for i in $L ; do
  NM=`echo $i | awk '{print substr($0,length($0)-9);}'`
  echo $NM
done
