#!/bin/bash

gcc mark8_barber.c -o barber -lrt -lpthread
gcc mark8_client.c -o client -lrt -lpthread

current_dir=$(pwd)

gnome-terminal -- bash -c "${current_dir}/barber; exec bash"
sleep 1

for i in {1..20}
do
    gnome-terminal -- bash -c "${current_dir}/client; exec bash"
    sleep 1
done
