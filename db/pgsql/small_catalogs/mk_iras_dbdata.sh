#!/bin/sh

LANG=C

OBJID=2117000000
OUTFILE=iras_all.db.txt

STEP=10
BEGIN=0
END=`expr $BEGIN + $STEP`
while [ $BEGIN != 360 ]; do

  echo "Downloading data from $BEGIN to $END ..."

  wget -q -O _tmp_${BEGIN}-${END}.txt 'http://irsa.ipac.caltech.edu/cgi-bin/Gator/nph-query?outfmt=1&server=PUBLIC_SERVER&database=iraspsc&catalog=iraspsc&spatial=None&submit=Submit+Query&selcols=pscname,ra,dec,semimajor,semiminor,posang,nhcon,fnu_12,fnu_25,fnu_60,fnu_100,fqual_12,fqual_25,fqual_60,fqual_100,nlrs,lrschar,relunc_12,relunc_25,relunc_60,relunc_100,tsnr_12,tsnr_25,tsnr_60,tsnr_100,cc_12,cc_25,cc_60,cc_100,var,disc,confuse,pnearh,pnearw,ses1_12,ses1_25,ses1_60,ses1_100,ses2_12,ses2_25,ses2_60,ses2_100,hsdflag,cirr1,cirr2,cirr3,nid,idtype,mhcon,fcor_12,fcor_25,fcor_60,fcor_100,rat_12_25,err_12_25,rat_25_60,err_25_60,rat_60_100,err_60_100&constraints='"${BEGIN}+%3C=+ra+AND+ra+%3C+${END}"

  BEGIN=$END
  END=`expr $END + $STEP`

done

rm -f $OUTFILE
BEGIN=0
END=`expr $BEGIN + $STEP`
while [ $BEGIN != 360 ]; do

  cat _tmp_${BEGIN}-${END}.txt | grep -v '^[\\|]' | sed -e 's/^[ ][ ]*//' -e 's/[ ]nul[ ]/ null /g' -e 's/\(^[^ ][^ ]*[ ][ ]*[^ ][^ ]*[ ][ ]*[^ ][^ ]*[ ][ ]*\)\([^ ][^ ]*[ ][ ]*[^ ][^ ]*[ ][ ]*\)\(.*\)/\1\3 0 0 0/' | tr -s ' ' '\t' >> _$OUTFILE
  rm -f _tmp_${BEGIN}-${END}.txt

  BEGIN=$END
  END=`expr $END + $STEP`
  
done
cat _$OUTFILE | awk '{printf("%d\t%s\n",'"$OBJID"'+NR,$0);}' > $OUTFILE
rm -f _$OUTFILE


echo "Number of line: "
wc -l $OUTFILE
