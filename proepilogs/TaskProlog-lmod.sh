#!/bin/bash

declare -A origenv

for var in $(compgen -e); do
    origenv[$var]="${!var}"
done

module purge
[[ -e /etc/lmod/lmodrc ]] && . /etc/lmod/lmodrc
[[ -e ~/.lmodrc ]] && . ~/.lmodrc

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
