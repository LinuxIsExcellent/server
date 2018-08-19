#!/bin/sh

pkill -2 front_server
sleep 5
pkill -2 center_server
sleep 15
pkill -2 map_server

