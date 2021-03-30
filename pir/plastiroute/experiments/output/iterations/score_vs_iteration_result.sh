#!/bin/bash

benchmarks=(BlackScholes DotProduct OuterProduct)
routing_algs=(route_dor route_bdor route_min_local route_directed_valient route_valient route_min_valient)

for benchmark in ${benchmarks[*]}; do
	for routing in ${routing_algs[*]}; do
		awk -v b=$benchmark -v r=$routing '{if(($5 == r) && ($3 == b)) print $7,"\t",$13}' score_vs_iteration.txt >> "${routing}_${benchmark}.dat"
	done
done
