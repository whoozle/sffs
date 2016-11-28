#!/bin/bash

python -c 'print "*"*50'
printf "Final Demo for CS307 at Purdue University.\n\nPress enter to iterate through the steps\n"
python -c 'print "*"*50'

printf "\n"

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
  ../../bin/yffs-create iso.img 100000
fi

printf "\nAdding Encrypted Files"
read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
#Encrypted Files	
	echo;
	
	echo "File Contents 123. " > FileA;
	echo "This is another file. " > TesterQ;
	echo "This is the third files content. " > etcXYZ;
	echo "This is the fourth test file." > AnothaOne;
	echo "41dd3n c0nt3nt. " > Documentz;
	echo "final TEST file. " > Filename;
	echo "This is unencrypted contet. " > Plainfile;
	
	../../bin/yffs-add iso.img FileA -1;
	../../bin/yffs-add iso.img TesterQ -1;
	../../bin/yffs-add iso.img etcXYZ -1;
	../../bin/yffs-add iso.img AnothaOne -1;	
	../../bin/yffs-add iso.img Documentz -1;
	../../bin/yffs-add iso.img Filename -1;
	../../bin/yffs-add iso.img Plainfile;
	
	python -c 'print "*"*25';
	printf "After Encryption\n";
	python -c 'print "*"*25';
	
  ../../bin/yffs-ls iso.img; echo; echo;
fi

printf "Reading Encrypted Files"
read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
#Encrypted Files	
	echo;
	
	echo "yffs-cat iso.img FileA -1;";
	../../bin/yffs-cat iso.img FileA -1; echo;
	echo "yffs-cat iso.img FileB -1;";
	../../bin/yffs-cat iso.img TesterQ -1; echo;
	echo "yffs-cat iso.img FileC -1;";
	 ../../bin/yffs-cat iso.img etcXYZ -1; echo;
	echo "yffs-cat iso.img FileD -1;";
	../../bin/yffs-cat iso.img AnothaOne -1; echo;	
	echo "yffs-cat iso.img FileE -1;";
	../../bin/yffs-cat iso.img Documentz -1; echo;
	echo "yffs-cat iso.img FileF -1;";
	../../bin/yffs-cat iso.img Filename -1; echo;
	echo "yffs-cat iso.img PlainFile;";
	../../bin/yffs-cat iso.img Plainfile; echo;
fi
	

read -p "[Enter]" var
if [ ${#var} -eq 0 ]; then
#Showing fs	
	echo;
	hexdump -C iso.img > fsContents;
	xxd -b iso.img > ffsContents;
	cat fsContents;
fi

	echo;echo;
	python -c 'print "*"*25'
	printf "Demo completed\n"
	python -c 'print "*"*25'
	echo;
	read -p "[Enter to clean and end]"
	rm iso.img
	rm File* f*sContents Tester* etc* AnothaOne Documentz Plain*;
	rm -rf *Folder*
