#!/usr/local/bin/python3.8
# encoding: utf-8

import pexpect
import sys
import random

q_order = 'q'
p_order = 'p'
d_order = 'd'
i_order = 'i'

common = [".*quit]:", pexpect.EOF, pexpect.TIMEOUT]
add = [".*insert:", pexpect.EOF, pexpect.TIMEOUT]
remove = [".*delete:", pexpect.EOF, pexpect.TIMEOUT]

prompt = [common,add, remove, pexpect.EOF, pexpect.TIMEOUT]

def insert(value):
    index = child.expect(common)
    if(0 == index):
        child.sendline(i_order)
        index = child.expect(add)
        if(0 == index):
            child.sendline(str(value))
        else:
            print("timeout!")
    else:
        show()
        print("can not insert node!")

def delete(value):
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

def show():
    index = child.expect(common)
    if(0 == index):
        child.sendline(p_order)
    else:
        print("can not show!")

def quit_proc():
    index = child.expect(common)
    if(0 == index):
        child.sendline(q_order)
    else:
        print("can not quit!")


def start_proc(cmd):
    child = pexpect.spawn(cmd)
    return child

def main():
    show()
    fout = open("log.txt", "+wb")
    child.logfile = fout
    for i in range(1, 100):
        value = random.randint(3,1000)
        insert(value)
    show()
    quit_proc()



if __name__=='__main__':
    cmd = './btree'
    child = start_proc(cmd)
    main()

