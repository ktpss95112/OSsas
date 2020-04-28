#!/usr/bin/env bash

mkdir -p output

for full_path in OS_PJ1_Test/*.txt ; do
    file=`basename -s .txt "${full_path}"`
    echo -n "running ${file} ... "
    
    sudo dmesg -C
    sudo ./main.out < "OS_PJ1_Test/${file}.txt" > "output/${file}_stdout.txt"
    dmesg | grep Project1 > "output/${file}_dmesg.txt"
    echo "done."
done
