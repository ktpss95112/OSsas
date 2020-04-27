#!/usr/bin/env bash

mkdir -p output

for full_path in testcases/*.txt ; do
    IFS='/' read -ra tmp <<< "$full_path"
    IFS='.txt' read -ra tmp2 <<< "${tmp[1]}"
    file="${tmp2[0]}"
    echo -n "running ${file} ... "
    sudo ./main.out < "testcases/${file}.txt" > "output/${file}_stdout.txt"
    dmesg | grep Project1 > "output/${file}_dmesg.txt"
    echo "done."
done
