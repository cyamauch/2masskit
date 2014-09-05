#!/bin/sh

cat $1 | tr '\t' ' ' | tr ',' ' ' | grep '^[ ][ ]*[a-zA-Z][a-zA-Z_0-9]*' \
 | awk '{printf(" <tr>\n  <td>%s</td><td>%s</td><td></td>\n  <td></td>\n  <td>\n   .\n  </td>\n </tr>\n",$1,$2);}'
