#!/bin/bash

rm -rf log.txt
./gcc.sh
python spawntest.py &
sleep 1
tailf log.txt
