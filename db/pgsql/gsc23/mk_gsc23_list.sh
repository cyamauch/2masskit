#!/bin/sh

if [ "$1" = "" ]; then
  echo "Specify directory."
  exit 1
fi

L=`find $1`
#LS=`echo "$L" | sort -r | grep 'S[0-9]*[.]fit'`
#LN=`echo "$L" | sort | grep 'N[0-9]*[.]fit'`

LS_A=`echo "$L" | grep 'S./2./../S' | sort -r | grep '[SN][0-9]*[.]fit'`
LS_B=`echo "$L" | grep 'S./3./../S' | sort -r | grep '[SN][0-9]*[.]fit'`
LS_C=`echo "$L" | grep 'S./1./../S' | sort -r | grep '[SN][0-9]*[.]fit'`
LS_D=`echo "$L" | grep 'S./0./../S' | sort -r | grep '[SN][0-9]*[.]fit'`

LN_A=`echo "$L" | grep 'N./0./../N' | sort | grep '[SN][0-9]*[.]fit'`
LN_B=`echo "$L" | grep 'N./1./../N' | sort | grep '[SN][0-9]*[.]fit'`
LN_C=`echo "$L" | grep 'N./3./../N' | sort | grep '[SN][0-9]*[.]fit'`
LN_D=`echo "$L" | grep 'N./2./../N' | sort | grep '[SN][0-9]*[.]fit'`

for i in $LS_A $LS_B $LS_C $LS_D $LN_A $LN_B $LN_C $LN_D ; do
  NM=`echo $i | awk '{print substr($0,length($0)-20);}'`
  echo $NM
done
