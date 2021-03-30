# Author: Gedeon Nyengele
# Description:
#	this script formats the records from the "score/VC vs routing algorithm" experiment.
#   it expects the following fields (in order) preprended to the output of "exp_parser.awk"
#   - experiment name
#   - benchmark name
# 	- routing function used

{
	print $1,"\tbenchmark","\t",$2,"\trouting","\t",$3,\
	"\tvc","\t",$4,\
	"\tarbiters","\t",$5,\
	"\tscore","\t",$6,\
	"\tmin_score","\t",$7,\
	"\tmax_score","\t",$8,\
	"\tmean_score","\t",$9,\
	"\tscore_param1","\t",$10,\
	"\tscore_param2","\t",$11,\
	"\tscore_param3","\t",$12;
}
