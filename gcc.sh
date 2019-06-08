#!/bin/bash

rm -rf btree
gcc btree.c vector.c -g -fstack-protector-all -o btree
