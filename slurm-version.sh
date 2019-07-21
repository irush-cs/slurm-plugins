#!/bin/bash

tmp=`mktemp /tmp/slurm-version.XXXXXX`;
/bin/echo "
#include <slurm/slurm.h>
#include <stdio.h>
int main() {
printf(\"%i.%02i.%i\\n\", SLURM_VERSION_MAJOR(SLURM_VERSION_NUMBER), SLURM_VERSION_MINOR(SLURM_VERSION_NUMBER), SLURM_VERSION_MICRO(SLURM_VERSION_NUMBER));
return 0;
};" >| $tmp

gcc -x c -Wall $tmp -o $tmp.exe 
[ -e $tmp.exe ] && $tmp.exe
rm -f $tmp $tmp.exe
