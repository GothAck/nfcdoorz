#!/bin/bash
tmp=$(tempfile)

cpp $2 \
  | grep -vE '^#' \
  | grep -vE '^$$' \
  | awk 'BEGIN {l=""} { if ($$0 ~ /;$$/) { print l " " $$0; l="" } else { l = l " " $$0 } }' \
  | sed 's|  *| |g' \
  | sed 's|^ +||g' \
  > $tmp
cat $1 | while read sym
do
  grep -E "\\b$sym\\b" $tmp
done | sed 's|^ *||g' | grep -vE '^extern '
rm $tmp
exit 0
