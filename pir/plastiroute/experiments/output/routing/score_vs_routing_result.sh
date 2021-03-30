#!/bin/bash

benchmarks=(BlackScholes DotProduct OuterProduct)
routing_algs=(route_dor route_bdor route_min_local route_directed_valient route_valient route_min_valient)

for benchmark in ${benchmarks[*]}; do
	awk -v b=$benchmark '{if($3 == b) print $5,"\t",$11,"\t",$19,"\t",$21,"\t",$23}' score_vs_routing.txt >> "data_score_vs_routing_${benchmark}.dat"
done
