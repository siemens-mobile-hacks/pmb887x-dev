#!/bin/bash
for file in */Makefile
do
	dir=$(dirname "$file")
	echo "PROJECT: $dir"
	make -C "$dir" $@ || exit 1
	echo ""
done
