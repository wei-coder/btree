#!/usr/bin/env python3
# encoding: utf-8

import sys

def parse_log(fd):
    fd.seek(0,0)
    arr = []
    dupnum = 0
    missnum = 0
    insertnum = 0
    i_total = 0
    deletenum = 0
    insert_invalid = 0
    delete_invalid = 0
    d_total = 0
    line = fd.readline()
    while line:
        idx = line.find("please input node id which you want insert:")
        if idx != -1:
            idx = line.find(":")
            num = int(line[idx+1:])
            arr.append(num)
        if line.find("The key you want insert is exist!") != -1:
            dupnum += 1
        if line.find("The key you want delete is not exist!") != -1:
            missnum += 1
        idx1 = line.find("datas")
        if idx != -1:
            idx2 = line.find("B+ Tree have")
            if idx2 != -1:
                total = int(line[idx2+13:idx1])
                if i_total < total:
                    d_total = i_total
                    i_total = total
                else:
                    d_total = total
        if line.find("success!") != -1:
            if line.find("Insert") != -1:
                insertnum += 1
            elif line.find("Delete") != -1:
                deletenum += 1
        if line.find("invalid") != -1:
            if line.find("Insert") != -1:
                insert_invalid += 1
            elif line.find("Delete") != -1:
                delete_invalid += 1
        line = fd.readline()
    for i in arr:
        if arr.count(i) >= 2:
            print("there is a duplicate value: %d"%i)
            arr.remove(i)
    print("dupnum:%d"%dupnum)
    print("insertnum:%d"%insertnum)
    print("insert invalid number:%d"%insert_invalid)
    print("Insert total %d datas"%i_total)
    print("missnum:%d"%missnum)
    print("deletenum:%d"%deletenum)
    print("delete invalid number:%d"%delete_invalid)
    print("delete remain %d datas"%d_total)

def check_num(fd,num):
    numstr = str(num)
    fd.seek(0, 0)
    line = fd.readline()
    while line:
        if line.find(numstr) != -1:
#            print(numstr + " find in :" + line)
            return numstr
#        print("%s have no %s"%(line, num));
        line = fd.readline()
    return None

if __name__ == "__main__":
    dump_num = sys.argv[1]
    logfile = open("log.txt", "+r")
    dumpfile = open(dump_num, "+r")
    line = logfile.readline()
    while line:
        idx = line.find("please input node id which you want insert:")
        if idx != -1:
            idx = line.find(":")
            num = int(line[idx+1:])
            #print(num)
            if check_num(dumpfile,num) == None:
                print("%d have not find"%num)
        line = logfile.readline()
    logfile.seek(0, 0)
    parse_log(logfile)
    logfile.close()
    dumpfile.close()
