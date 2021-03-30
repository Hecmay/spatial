#!/bin/bash

ITER_PARAMS="-i100 -p100 -t1 -d100 -q4 -x0 -e4 "
TAG=v0s4
IN_DIR=/home/acrucker/finals3/
OUT_DIR=$IN_DIR
COMMIT=`git rev-parse --short HEAD`
CSV=results.csv_${COMMIT}_${TAG}
LOG=results.log_${COMMIT}_${TAG}

rm $CSV
rm $LOG
for bmark in `ls $IN_DIR | grep D_v0_s0 | grep Product`; do
#for bmark in `ls $IN_DIR | grep Scholes`; do
#for bmark in `ls finals`; do
  #for static in 0 1 2; do
  for static in 1 ; do
    echo "$bmark ($static)";
#    ./plastiroute -r 16 \
#                  -c 8 \
#                  -s 0 \
#                  -n $IN_DIR/$bmark/node.csv \
#                  -l $IN_DIR/$bmark/link.csv \
#                  -g $OUT_DIR/$bmark/$static.dot \
#                  -x $static \
#                  -o $OUT_DIR/$bmark/$static.place \
#                  -v results.csv \
#                  -k "$bmark.$static" \
#                  $ITER_PARAMS
#    ./plastiroute -r 16 \
#                  -c 8 \
#                  -s 0 \
#                  -e 6 \
#                  -n $IN_DIR/$bmark/node.csv \
#                  -l $IN_DIR/$bmark/link.csv \
#                  -g $OUT_DIR/$bmark/$static.S6.dot \
#                  -x $static \
#                  -o $OUT_DIR/$bmark/$static.S6.place \
#                  -v results.csv \
#                  -k "$bmark.$static.S6" \
#                  $ITER_PARAMS
    echo "./plastiroute -r 16 \
                  -c 8 \
                  -s 0 \
                  -e 4 \
                  -n $IN_DIR/$bmark/node.csv \
                  -l $IN_DIR/$bmark/link.csv \
                  -g $OUT_DIR/$bmark/$TAG.dot \
                  -x $static \
                  -o $OUT_DIR/$bmark/$TAG.place \
                  -v $CSV \
                  -a route_min_directed_valient \
                  -k $bmark.$TAG \
                  $ITER_PARAMS" #| tee -a $LOG
    ./plastiroute -r 16 \
                  -c 8 \
                  -s 0 \
                  -e 4 \
                  -n $IN_DIR/$bmark/node.csv \
                  -l $IN_DIR/$bmark/link.csv \
                  -g $OUT_DIR/$bmark/$TAG.dot \
                  -x $static \
                  -o $OUT_DIR/$bmark/$TAG.place \
                  -v $CSV \
                  -a route_min_directed_valient \
                  -k "$bmark.$TAG" \
                  $ITER_PARAMS 2>&1 | tee -a $LOG
                  #-g $OUT_DIR/$bmark/$static.S6.A.dot \
  done
done

cat $CSV | column -ts ','
