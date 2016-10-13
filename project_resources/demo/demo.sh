#!/bin/bash

#Author: Wyatt Dahlenburg (wdahlenb@purdue.edu)
#Date: 10/13/16

python -c 'print "*"*50'
printf "Demo for CS307 at Purdue University.\n\nPress enter to iterate through the steps\n"
python -c 'print "*"*50'

printf "\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  yffs-create iso.img 500000
fi

printf "\nInserting files into image\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  echo "Creating files"
  for i in {000..100}
    do
      echo "File${i}" > "File${i}.txt"
      yffs-edit iso.img "File${i}.txt"
      rm "File${i}.txt"
    done  
fi

printf "\nListing files\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then 
  yffs-ls iso.img
fi

printf "\nRemoving odds from image\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  for i in {000..100}
    do
      if [ $((i%2)) -ne 0 ]; then
	yffs-rm iso.img remove "File${i}.txt"
      fi
    done
fi

printf "\nListing files\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  yffs-ls iso.img
fi

printf "\nReading all files divisible by 10\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  for i in {000..100}
    do
      if [ $((i%10)) -eq 0 ]; then
        yffs-cat iso.img "File${i}.txt"
      fi
    done
fi

printf "\nUpdating the first 5 files\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  for i in {00..10}
    do
      #We have to take the even files because we removed the odds 
      if [ $((i%2)) -eq 0 ]; then
        echo "Modified ${i}" > "File${i}.txt"
        yffs-edit iso.img "File${i}.txt" >/dev/null
        rm "File${i}.txt"
        yffs-cat iso.img "File${i}.txt"
      fi
    done
fi

printf "Demo completed (Cleaning up)\n"
rm iso.img

