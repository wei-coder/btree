#!/usr/bin/env python
# encoding: utf-8

from pexpect import *

q_order = 'q\r'
p_order = 'p\r'
d_order = 'd\r'
i_order = 'i\r'

common = 'quit]:'
insert = 'insert:'
delete = 'delete:'

#cmd = '/bin/bash -c "./btree"'
child = run('./btree',events={common:p_order})
fout = open('log.txt',"w")
child.logfile = fout
fout.close()
#print(child.before)
#child.close()

