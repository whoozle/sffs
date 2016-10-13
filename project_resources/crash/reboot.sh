#!/bin/bash
#Author: Wyatt Dahlenburg (wdahlenb@purdue.edu)

#Hard coded value
#This was determined by running:
#
#	time yffs-write iso.img file.txt
#
#	output:
#	real	0m1.198s
#	user	0m0.000s
#	sys	0m0.230s
#
# 
sleep 0.5

#I chose a value that would be somewhere in the middle of the write operation

#echo "Rebooting now"

sudo reboot

echo "Rebooting"

#Device should be off
