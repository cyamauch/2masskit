#!/bin/sh

BASE=$1

if [ "$1" = "" ]; then
  echo "Specify URL"
  echo "[Example]"
  echo "$ sh 2mass_download.sh http://darts.jaxa.jp/pub/ir/2masskit/2mass/"
else

  for i in `cat 2mass_md5.txt|awk '{print $1"#"$2}'`; do
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
