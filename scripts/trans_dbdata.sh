#!/bin/sh

if [ "$1" = "" ]; then
  echo "Specify dir."
  exit 0
fi

LDIR=$1
SDIR=/nasA_darts1/data/ir/2masskit/v2
GZIP=0

LCAT=`ls $LDIR | grep '[.]db[.]txt$' | head -n 1 | sed -e 's/_.*//g'`
if [ "$LCAT" = "" ]; then
  LCAT=`ls $LDIR | grep '[.]db[.]txt[.]gz$' | head -n 1 | sed -e 's/_.*//g'`
  GZIP=1
fi

echo "Catalog name is [$LCAT]."
echo "OK?"
read ANS

if [ "$ANS" = "y" -a $GZIP = 0 ]; then

  rm -rf _tmp_
  mkdir _tmp_

  LIST=`/bin/ls $LDIR | grep '[.]db[.]txt$'`
  for i in $LIST ; do
    echo "compressing ... $i => _tmp_/$i.gz"
    gzip -c $i > _tmp_/$i.gz
  done
fi

if [ $GZIP = 0 ]; then
  GZDIR="_tmp_"
else
  GZDIR="$LDIR"
fi

echo "Connect: ssh -g -L 20021:133.74.198.4:22 koala.ir.isas.jaxa.jp"
echo "OK?"
read ANS

if [ "$ANS" = "y" ]; then

  cd $GZDIR
  echo "tar cf - . | ssh -p 20021 cyamauch@localhost \"gtar xmf - -C $SDIR/$LCAT\""
  tar cf - . | ssh -p 20021 cyamauch@localhost "gtar xmf - -C $SDIR/$LCAT"

fi

