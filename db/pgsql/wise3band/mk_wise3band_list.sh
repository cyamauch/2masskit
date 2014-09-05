#!/bin/sh

if [ "$1" = "" ]; then
  echo "Specify directory."
  exit 1
fi

L=`find $1 | sort | grep 'wise-3band-wdb-part..[.]'`

for i in $L ; do
  NM=`echo $i | sed -e 's|.*/||' -e 's|[.][^.][^.]*$||'`
  echo $NM
done
