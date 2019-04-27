#!/usr/local/bin/python3.8
# encoding: utf-8

import pexpect
import sys
import random
import time
import os

q_order = 'q'
p_order = 'p'
d_order = 'd'
i_order = 'i'

common = [".*quit]:", pexpect.EOF, pexpect.TIMEOUT]
add = [".*insert:", pexpect.EOF, pexpect.TIMEOUT]
remove = [".*delete:", pexpect.EOF, pexpect.TIMEOUT]

prompt = [common,add, remove, pexpect.EOF, pexpect.TIMEOUT]

def insert(child, value):
    index = child.expect(common)
    if(0 == index):
        child.sendline(i_order)
        index = child.expect(add)
        if(0 == index):
            child.sendline(str(value))
        else:
            print("timeout!")
    else:
        show(child)
        print("can not insert node!")

def delete(child, value):
    index = child.expect(common)
    if(0 == index):
        child.sendline(d_order)
        index = child.expect(remove)
        if(0 == index):
            child.sendline(str(value))
        else:
            print("timeout!")
    else:
        print("can not delete node!")

def show(child):
    index = child.expect(common)
    if(0 == index):
        child.sendline(p_order)
    else:
        print("can not show!")

def quit_proc(child):
    child.interact()


def start_proc(cmd):
    child = pexpect.spawn(cmd)
    return child

def main_test(child,fout):
    show(child)
    child.logfile = fout
    for i in range(0, 120):
#        value = random.randint(3,1000)
        value = i
        insert(child,value)
        time.sleep(0.001)
    show(child)



if __name__=='__main__':
    os.system('rm -rf core.*')
    os.system('rm -rf *.txt')
    os.system('./gcc.sh')
    fout = open("log.txt", "+wb")
    os.system("tail -f log.txt &")
    cmd = './btree'
    child = start_proc(cmd)
    main_test(child,fout)
    os.system("ps -ef | grep tail | grep -v grep | cut -c 9-15 | xargs kill -9")
    quit_proc(child)

