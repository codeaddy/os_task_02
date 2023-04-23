#!/bin/bash

gcc mark8_barber.c -o barber -lpthread
gcc mark8_client.c -o client -lpthread

current_dir=$(pwd)

osascript -e "tell application \"Terminal\" to do script \"${current_dir}/barber\""
sleep 1

for i in {1..20}
do
    osascript -e "tell application \"Terminal\" to do script \"${current_dir}/client\""
    sleep 1
done
