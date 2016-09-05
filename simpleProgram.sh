#!/bin/bash
echo "Hello World" > test
echo "File A" > fileA
echo "File B" > fileB
./sffs-tool sampleImg.iso createfs 10000
printf "\nWriting file test\n"
./sffs-tool sampleImg.iso write test
rm test
printf "\nReading test\n"
./sffs-tool sampleImg.iso read test
printf "\nWriting fileA\n"
./sffs-tool sampleImg.iso write fileA
printf "\nWriting fileB\n"
./sffs-tool sampleImg.iso write fileB
rm fileA
rm fileB
printf "\nReading fileA\n"
./sffs-tool sampleImg.iso read fileA
printf "\nReading fileB\n"
./sffs-tool sampleImg.iso read fileB
rm sampleImg.iso
