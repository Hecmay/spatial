# Author: Gedeon Nyengele
# Description:
#	this script formats the records from the "score vs iteration" experiment.
#   it expects the following fields (in order) preprended to the output of "exp_parser.awk"
#   - experiment name
#   - benchmark name
# 	- routing function used
# 	- # of iterations

{
	print $1,"\tbenchmark","\t",$2,"\trouting","\t",$3,"\titerations","\t",$4,\
	"\tvc","\t",$5,\
	"\tarbiters","\t",$6,\
	"\tscore","\t",$7,\
	"\tmin_score","\t",$8,\
	"\tmax_score","\t",$9,\
	"\tmean_score","\t",$10,\
	"\tscore_param1","\t",$11,\
	"\tscore_param2","\t",$12,\
	"\tscore_param3","\t",$13;
}
