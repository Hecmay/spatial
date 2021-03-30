#!/bin/bash

benchmarks=(BlackScholes DotProduct OuterProduct)
routing_algs=(route_dor route_bdor route_min_local route_directed_valient route_valient route_min_valient)

for benchmark in ${benchmarks[*]}; do
	awk -v b=$benchmark '{if($3 == b) print $5,"\t",$7}' score_vs_routing.txt >> "vc_${benchmark}.dat"
done
