#!/bin/bash

set -e
set -u

for node in `sinfo -o '%N' -N -h`; do
    if [[ `scontrol show node $node | tr ' ' '\n' | grep -P '^Gres=.*\bgpu\b|^ActiveFeatures=.*\bcpuonly\b' | wc -l` != 1 ]]; then
        echo $node is either missing Gres=gpu or Feature=cpuonly 1>&2
    fi
done

