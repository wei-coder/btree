#!/bin/bash

./gcc.sh
python spawntest.py &
tailf log.txt
