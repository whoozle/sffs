#!/bin/bash
#Author: Wyatt Dahlenburg (wdahlenb@purdue.edu)

#Sample file can be found at http://dazzlepod.com/site_media/txt/passwords.txt

echo "Starting script"

FILE="passwords.txt"

if [ -f $FILE ];
then
	echo "File $FILE found"
else
	echo "File $FILE not found. Starting download"
	wget http://dazzlepod.com/site_media/txt/passwords.txt -q --show-progress 
	wait
	
	#Reorder reboot command due to issues with race cases
	yffs-create iso.img 25000000
	
	yffs-add iso.img passwords.txt & sudo reboot && fg
	exit
fi

#Create the image file with enough room for the passwords file
yffs-create iso.img 25000000

#Run the reboot script in the backround and begin to write the passwords file
sh ./reboot.sh & yffs-add iso.img passwords.txt && fg

#Shouldn't happen if reboot is properly scheduled
echo "Finished"

