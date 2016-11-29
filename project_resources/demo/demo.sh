#!/bin/bash

#Author: Wyatt Dahlenburg (wdahlenb@purdue.edu)
#Date: 10/13/16

python -c 'print "*"*50'
printf "Demo for CS307 at Purdue University.\n\nPress enter to iterate through the steps\n"
python -c 'print "*"*50'

printf "\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  ../../bin/yffs-create iso.img 500000
fi

printf "\nInserting files into image\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  echo "Creating files"
  for i in {0..100}
    do
     echo "File${i}" > "File${i}.txt"
      ../../bin/yffs-add iso.img "File${i}.txt"
    done  
fi

#printf "\nInserting files in folders into image\n"
#
#read -p "[Enter]" var
#if [ ${#var} -eq 0 ]; then
#  echo "Creating files"
#  for i in {0..100}
#    do
#      echo "File${i}" > "File${i}.txt"
#      ../../bin/yffs-add iso.img "testfolder/File${i}.txt"
#    done  
#fi


printf "\nListing files\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then 
  ../../bin/yffs-ls iso.img
fi

printf "\nRemoving odds from image\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  for i in {0..100}
    do
      if [ $((i%2)) -ne 0 ]; then
	../../bin/yffs-rm iso.img "File${i}.txt"
      fi
    done
fi

printf "\nListing files\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  ../../bin/yffs-ls iso.img
fi

printf "\nReading all files divisible by 10\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  for i in {0..100}
    do
      if [ $((i%10)) -eq 0 ]; then
        ../../bin/yffs-cat iso.img "File${i}.txt"
      fi
    done
fi

printf "\nUpdating the first 5 files\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  for i in {0..10}
    do
      #We have to take the even files because we removed the odds 
      if [ $((i%2)) -eq 0 ]; then
        echo "mod*" > toAppend;
          ../../bin/yffs-edit iso.img "File${i}.txt" "toAppend"
        rm "toAppend"
        ../../bin/yffs-cat iso.img "File${i}.txt"
      fi
    done
fi

printf "Demo completed (Cleaning up)\n"
rm iso.img

