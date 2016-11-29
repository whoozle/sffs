#!/bin/bash

python -c 'print "*"*50'
printf "Final Demo for CS307 at Purdue University.\n\nPress enter to iterate through the steps\n"
python -c 'print "*"*50'

printf "\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
	echo "Creating a file system: yffs-create iso.img 10000";
	../../bin/yffs-create iso.img 500000
fi


read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  echo "Inserting files into image: yffs-add iso.img Filename";
  for i in {0..25}
    do
     echo "File${i}" > "File${i}.txt"
      ../../bin/yffs-add iso.img "File${i}.txt"
    done  
fi

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then 
  echo "Listing files in image: yffs-ls iso.img";
  ../../bin/yffs-ls iso.img
fi


read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
	echo "Removing odd files in image: yffs-rm iso.img FileX";
	for i in {0..25}; do
    	  if [ $((i%2)) -ne 0 ]; then
		../../bin/yffs-rm iso.img "File${i}.txt"
      	  fi
    	done
fi

printf "\nReading all files divisible by 10\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  echo "Reading all files divisible by 10: yffs-cat iso.img FileX ";
  for i in {0..25}
    do
      if [ $((i%10)) -eq 0 ]; then
        ../../bin/yffs-cat iso.img "File${i}.txt"
      fi
    done
fi


read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  echo "Updating the first 5 files: yffs-edit iso.img FileX " New Text "";
  for i in {0..10}
    do
      #We have to take the even files because we removed the odds 
      if [ $((i%2)) -eq 0 ]; then
	python -c 'print "*"*25'
        echo "New Text for File${i} " > toAppend;
        ../../bin/yffs-edit iso.img "File${i}.txt" "toAppend"
	rm "toAppend"
	../../bin/yffs-cat iso.img "File${i}.txt"
      fi
    done
fi

python -c 'print "*"*25'
printf "Creating Test Directorys\n yffs-add iso.img \"TestFileA\" \"FolderA\" -1\n yffs-add iso.img \"TestFileB\" \"FolderB\" ";
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

read -p "[Enter]"
if [ ${#var} -eq 0 ]; then
  echo "Listing files in image: yffs-ls iso.img";
  ../../bin/yffs-ls iso.img
fi

printf "\nEditing File Owners "
read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  echo "Editing File Owners in image: yffs-chown iso.img File0.txt";
	#CHOWN	
  ../../bin/yffs-chown iso.img File0.txt UserA
  ../../bin/yffs-chown iso.img File10.txt UserB
  ../../bin/yffs-ls iso.img | head -n4 | tail -n4; echo; echo;
fi

printf "\nEditing File Permissions "
read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
#CHMOD
  echo "Editing File Permissions in image: yffs-chown iso.img File0.txt";
  ../../bin/yffs-chmod iso.img File4.txt 10
  ../../bin/yffs-chmod iso.img File6.txt 10
  ../../bin/yffs-chmod iso.img File8.txt 10
  ../../bin/yffs-chmod iso.img GpmefsB0UftuGjmfB 10
  ../../bin/yffs-ls iso.img | tail -n5; echo; echo;
fi

python -c 'print "*"*25'
printf "Filesystem Content\n"
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
  echo -n "GpmefsB0UftuGjmfB -> ";ls Folder*
fi

echo;echo;
python -c 'print "*"*25'
printf "Demo completed\n"
python -c 'print "*"*25'
echo;
read -p "[Enter to clean and end]"
rm iso.img
rm File*;
rm -rf *Folder*
