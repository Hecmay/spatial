#!/bin/bash
# Author: Gedeon Nyengele
# Description:
#		this script provides utils used by other scripts
# ======================================================

# printing text with color
C_RED="\033[31m"
C_GREEN="\033[32m"
C_BLUE="\033[34m"
C_RESET="\033[0m"

make_red()
{
	echo -e "${C_RED}$1${C_RESET}"
	return 0
}

make_green()
{
	echo -e "${C_GREEN}$1${C_RESET}"
	return 0
}

make_blue()
{
	echo -e "${C_BLUE}$1${C_RESET}"
	return 0
}

# prints error message
print_error()
{
	echo "$(make_red "[ERROR] $1")"
	return 0
}

# print success message
print_ok()
{
	echo "$(make_green $1)"
	return 0
}


# creates an output directoy if not already created
init_output_dir()
{
	if [ ! -d output ]; then
		mkdir output
	fi
	return 0
}

# backs up a file with an "-old" suffix
oldify()
{
	if [ -z $1 ]; then
		print_error "oldify() cannot be called without an argument!"
		return 1
	fi
	if [ ! -f $1 ]; then
		#print_error "file $1 does not exist!"
		return 1
	fi
	
	mv $1 "$(dirname $1)/$(basename $1).old"
	return 0
}
