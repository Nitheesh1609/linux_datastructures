#!/bin/bash

dmesg -C
sudo insmod project2.ko int_str="1,2,3,4,5"
dmesg
cat /proc/proj2
sudo rmmod project2.ko

