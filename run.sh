#!/bin/zsh

		# [[ $1 -ne 0 ]] && length=$1
for length in $(seq 3 333); do
	for rep in $(seq 1 100); do
		# while [ 1 ]; do
			cat /dev/urandom | tr -dc 'acgt' | fold -w $length | head -n 1 > random.txt
			./a.out -f random.txt || return 1
		# done
	done
done

