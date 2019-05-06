#!/usr/bin/env python3
# encoding: utf-8

import pexpect
import sys
import random
import time
import os

q_order = 'q'
p_order = 'p'
f_order = 'f'
s_order = 's'
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

def draw_tree(child):
    index = child.expect(common)
    if(0 == index):
        child.sendline(p_order)
    else:
        print("can not draw!")

def tree_dump(child):
    index = child.expect(common)
    if(0 == index):
        child.sendline(f_order)
    else:
        print("can not dump tree!")

def show_info(child):
    index = child.expect(common)
    if 0 == index:
        child.sendline(s_order)
    else:
        print("can not show tree info!")

def quit_proc(child):
    child.interact()


def start_proc(cmd):
    child = pexpect.spawn(cmd)
    return child

def batch_insert(child,num):
    for i in range(0, num):
        value = random.randint(3,1000)
#        value = i
        insert(child,value)
        time.sleep(0.001)

def batch_delete(child,num):
    data_file = open("tree_data.txt", "+r")
    line = data_file.readlines()
    for i in range(0, num):
        idx = random.randint(0,100)
#        value = i
        delete(child,int(line[idx]))
        time.sleep(0.001)


def print_prompt():
    print("Insert node please press 'i'!")
    print("Delete node please press 'd'!")
    print("Get prompt info please press '?'!")
    print("Print tree info please press 'p'!")
    print("quit this program please pree 'q'!")

def print_eof():
    print("#input order [insert/del/?/quit]:")

if __name__=='__main__':
    os.system('rm -rf core.*')
    os.system('rm -rf *.txt')
    os.system('./gcc.sh')
    fout = open("log.txt", "wb")
    os.system("tailf log.txt &")
    cmd = './btree'
    child = start_proc(cmd)
    child.logfile = fout
    print_prompt()
    while True:
        order = input()
        if order.find('i') != -1:
            mode = input("sigle/multiple:")
            if mode.find('s') != -1:
                key = input("input key:")
                insert(child, int(key))
                cout += 1
            elif mode.find('m') != -1:
                num = input("input key number:")
                batch_insert(child, int(num))
                tree_dump(child)
                draw_tree(child)
                show_info(child)
        elif order.find('d') != -1:
            mode = input("sigle/multiple:")
            if mode.find('s') != -1:
                key = input("input key:")
                delete(child, int(key))
            elif mode.find('m') != -1:
                num = input("input key number:")
                batch_delete(child, int(num))
                tree_dump(child)
                draw_tree(child)
                show_info(child)
        elif order.find('p') != -1:
            draw_tree(child)
        elif order.find('?') != -1:
            print_prompt()
        elif order.find('s') != -1:
            show_info(child)
        elif order.find('q') != -1:
            break
    os.system("ps -ef | grep tailf | grep -v grep | cut -c 9-15 | xargs kill -9")
    quit_proc(child)
    fout.close()
