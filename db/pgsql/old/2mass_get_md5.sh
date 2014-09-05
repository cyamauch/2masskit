#!/bin/sh

L=`ls | grep '2mass_psc_...[.]db[.]txt'`
for i in $L; do
  md5sum $i | tr '\t' ' ' | tr -s ' '
done

L=`ls | grep '2mass_psc_[^_]*_j2000i[.]db[.]txt'`
for i in $L; do
  md5sum $i | tr '\t' ' ' | tr -s ' '
done

L=`ls | grep '2mass_psc_[^_]*_d8_xyzi[.]db[.]txt'`
for i in $L; do
  md5sum $i | tr '\t' ' ' | tr -s ' '
done
