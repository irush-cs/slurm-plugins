#!/bin/bash

declare -A origenv

for var in $(compgen -e); do
    origenv[$var]="${!var}"
done

module purge
if ! [[ "$1" = "purge" ]]; then
    [[ -e /etc/lmod/lmodrc ]] && . /etc/lmod/lmodrc
    [[ -e ~/.lmodrc ]] && . ~/.lmodrc
fi

if [[ -n "$SPANK_MODULES" ]]; then
    for module in `echo $SPANK_MODULES | tr ',' ' '`; do
        module try-load $module
    done
fi

for var in $(compgen -e); do
    if [[ "${origenv[$var]}" != "${!var}" ]]; then
        echo "export ${var}=${!var}"
    fi
done

for var in "${!origenv[@]}"; do
    if [[ -z "${!var}" ]]; then
        echo "unset ${var}"
    fi
done
