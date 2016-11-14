#!/bin/bash

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
  for i in {0..25}
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


printf "\nListing Filesystem Contents\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then 
  ../../bin/yffs-ls iso.img
fi

printf "\nRemoving odds from image\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  for i in {0..25}
    do
      if [ $((i%2)) -ne 0 ]; then
	../../bin/yffs-rm iso.img "File${i}.txt"
      fi
    done
fi

printf "\nReading all files divisible by 10\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  for i in {0..25}
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
	python -c 'print "*"*25'
        echo "Contents File${i}.txt: "
        echo "Mod Text ${i} " > toAppend;
          ../../bin/yffs-edit iso.img "File${i}.txt" "toAppend"
	rm "toAppend"
	../../bin/yffs-cat iso.img "File${i}.txt"
      fi
    done
fi

python -c 'print "*"*25'
printf "Creating Test Directorys\n"
python -c 'print "*"*25'
mkdir FolderA
mkdir FolderB
mkdir LargeFolder
echo "ContentA" > FolderA/TestFileA
echo "Content1" > FolderB/TestFileA
echo "Content2" > FolderB/TestFileB
echo "Content3" > FolderB/TestFileC
echo "Content4" > FolderB/TestFileD

../../bin/yffs-add iso.img "FolderA/TestFileA" -1 > /dev/null
../../bin/yffs-add iso.img "FolderB/TestFileB" > /dev/null

printf "\nListing Filesystem Content "
read -p "[Enter]"
if [ ${#var} -eq 0 ]; then
  ../../bin/yffs-ls iso.img
fi

printf "\nEditing File Owners "
read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
#CHOWN	
	echo;
	python -c 'print "*"*25'
	printf "Before CHOWN\n"
	python -c 'print "*"*25'

  ../../bin/yffs-ls iso.img | head -n4 | tail -n3; echo; echo;
  ../../bin/yffs-chown iso.img File0.txt UserA
  ../../bin/yffs-chown iso.img File10.txt UserB
	
	python -c 'print "*"*25'
	printf "After CHOWN\n"
	python -c 'print "*"*25'
  
  ../../bin/yffs-ls iso.img | head -n4 | tail -n3; echo; echo;
fi
printf "\nEditing File Permissions "
read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
#CHMOD
	echo;
	python -c 'print "*"*25'
	printf "Before CHMOD\n"
	python -c 'print "*"*25'

  ../../bin/yffs-ls iso.img | tail -n5; echo; echo;
  ../../bin/yffs-chmod iso.img File4.txt 10
  ../../bin/yffs-chmod iso.img File6.txt 10
  ../../bin/yffs-chmod iso.img File8.txt 10
  ../../bin/yffs-chmod iso.img GpmefsB0UftuGjmfB 10
	
	python -c 'print "*"*25'
	printf "After CHMOD\n"
	python -c 'print "*"*25'
  
  ../../bin/yffs-ls iso.img | tail -n5; echo; echo;
fi

python -c 'print "*"*25'
printf "Final Filesystem Content\n"
python -c 'print "*"*25'

read -p "[Enter]"
if [ ${#var} -eq 0 ]; then
  ../../bin/yffs-ls iso.img
fi
	
echo;	
python -c 'print "*"*25'
printf "Printing Folder Contents\n"
python -c 'print "*"*25'
read -p "[Enter]"
if [ ${#var} -eq 0 ]; then
  ../../bin/yffs-ls iso.img FolderB; echo; echo;
  echo -n "GpmefsB0UftuGjmfB -> ";ls Folder*
fi

	echo;echo;
	python -c 'print "*"*25'
	printf "Demo completed (Cleaning up)\n"
	python -c 'print "*"*25'
	echo;
rm iso.img
rm File*
rm -rf *Folder*
