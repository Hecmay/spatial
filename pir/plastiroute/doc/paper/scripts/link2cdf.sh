#!/bin/bash

for b in BlackScholes DotProduct GDA GEMM_Blocked LogReg OuterProduct TPCHQ6;
do
  FILE=$PLASTISIM_HOME/configs/gen/$b/link.csv
  TOTAL=`cat $FILE | cut -f2 -d, | tail -n +2 | sort -rn | awk '{s+=$1} END {print s}'`
  LINES=`cat $FILE | tail -n +2 | wc -l`
  echo "0 0" > data/link_cdf/$b.dat
  cat $FILE | cut -f2 -d, | tail -n +2 | sort -rn | awk "{s+=\$1; l+=1; printf \"%f %f\\n\", l/$LINES, s/$TOTAL}" >> data/link_cdf/$b.dat
done
