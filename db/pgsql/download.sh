#!/bin/sh

BASE=$1
CAT=`basename $0 | sed -e 's/_download[.]sh//'`

if [ "$CAT" != "download.sh" ]; then
  if [ "$1" = "" ]; then
    echo "Specify URL"
    echo "[Example]"
    echo "$ sh $0 http://darts.jaxa.jp/pub/ir/2masskit/v2/$CAT/"
  else

    for i in `cat ${CAT}_md5.txt|awk '{print $1"#"$2}'`; do
      MD5=`echo $i | sed -e 's/#[^#]*//'`
      FILE=`echo $i | sed -e 's/[^#]*#//'`
      echo "Downloading $FILE ..."
      wget -O - "$BASE/$FILE.gz" | gzip -dc > $FILE
      if [ $? != 0 ]; then
        echo "[ERROR] wget failed\n"
        exit 1
      fi
    done
  fi
fi
