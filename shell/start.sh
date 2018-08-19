#!/bin/sh

./map_boot.sh &
sleep 40

./center_boot.sh &
sleep 20

./front_boot.sh &

