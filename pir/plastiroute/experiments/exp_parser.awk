# file: exp_parser.awk
# Author: Gedeon Nyengele
# Description:
#		parses the output of an experiment to obtain the following fiels (in order)
#		- vc count
# 		- arbiter count
#		- score
# 		- minimum score
# 		- maximum score
#		- mean score
# 		- score parameter 1
# 		- score parameter 2
# 		- score parameter 3

BEGIN {
	vc=0;
	arbiters=0;
	score=0;
	min_score=0;
	max_score=0;
	mean_score=0;
	score_param1=0;
	score_param2=0;
	score_param3=0;
}

{
	if ($2 == "scores:") {
		min_score=$4;
		max_score=$7;
		mean_score=$9;
		split($5, score_params, "/");
		score_param1=score_params[1];
		score_param2=score_params[2];
		score_param3=score_params[3];
	} else if ($1 == "Best") {
		score=$4;
	} else if ($1 == "Used") {
		vc=$2;
	} else if ($1 == "Number") {
		arbiters=$4;
	}
}

END {
	print vc,"\t",arbiters,"\t",score,"\t",min_score,"\t",max_score,"\t",mean_score,"\t",score_param1,"\t",score_param2,"\t",score_param3;
}
