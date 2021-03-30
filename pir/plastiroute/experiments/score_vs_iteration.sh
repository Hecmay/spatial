#!/bin/bash
# Author: Gedeon Nyengele
# Description:
#		this script runs the score vs iteration experiment
#		for all benchmark and for all routing functions
# =========================================================


source utils.sh

#########################
# experiment parameters #
#########################
exp_name=score_vs_iteration
benchmarks=(BlackScholes DotProduct GDA LogReg OuterProduct)
routing_algs=(route_dor route_bdor route_min_local route_directed_valient route_valient route_min_valient)
#routing_algs=(route_dor)
iterations=$(seq 100 200 1000)

# initialize output directory
init_output_dir

# backup output file
output_file="output/${exp_name}.txt"
oldify $output_file

# status tracking
benchmark_len=${#benchmarks[@]}
routing_len=${#routing_algs[@]}
iterations_len=$(echo $iterations | awk '{ print NF }')
total_len=`expr $benchmark_len \* $routing_len \* $iterations_len`
stat_counter=0
echo "Number of iterations = $total_len"
echo "Number of benchmarks = $benchmark_len"
echo "Number of routing algs  = $routing_len"

echo -n "$(make_green "[STATUS] ") $stat_counter %"

# experiment run
for benchmark in ${benchmarks[*]}; do
	for routing in ${routing_algs[*]}; do
		for iteration in ${iterations[*]}; do
			sim_res=$(../plastiroute -n tests/$benchmark/node.csv -l tests/$benchmark/link.csv -i $iteration -a $routing | awk -f exp_parser.awk)
			sim_res="$exp_name $benchmark $routing $iteration $sim_res"
			echo $sim_res | awk -f exp_format_score_vs_iteration.awk >> $output_file
			
			stat_counter=`expr $stat_counter + 1`
			stat_percent=`expr 100 \* $stat_counter / $total_len`
			echo -ne "\r$(make_green "[STATUS] ") ${stat_percent}% [benchmark: $benchmark, routing: $routing]                                             "
		done

	done
done
echo

