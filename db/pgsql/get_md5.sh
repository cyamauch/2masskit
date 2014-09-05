#!/bin/sh

L=`ls | sort | grep '[^_]*_main_[0-9a-z][0-9a-z]*[.]db[.]txt'`
for i in $L; do
  CAT=`echo $i | sed -e 's/_.*//'`
  break
done
F="${CAT}_md5.txt"
echo "Create $F ? [y/n]"
read ANS

echo "Creating $F ..."
if [ "$ANS" = "y" -o "$ANS" = "Y" ]; then
  rm -f $F
  for i in $L; do
    md5sum $i | tr '\t' ' ' | tr -s ' ' >> $F
  done

  L=`ls | sort | grep '[^_]*_eqi_[0-9a-z][0-9a-z]*[.]db[.]txt'`
  for i in $L; do
    md5sum $i | tr '\t' ' ' | tr -s ' ' >> $F
  done
fi

