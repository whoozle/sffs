# cs307





Simple Fault-tolerant File System, version 1

This tiny filesystem was designed to be the most safe in the world of SPI flash 
filesystems. Writing into such flash splitted into single-byte commands and 
could be interrupted by power failure or crash and therefore corrupt your data.
Sffs finalizes metadata with one-bit switch and won't lose anything.

Also this filesystem will protect you from flash wearing. It saves timestamps
of writes and always rotate files around the flash, so you never got the same
file written again and again on the same place.






Sync with master branch->
git rebase origin/master
git push

Push to master branch->
//Update local branch
git add -A
git commit
git push
//push to master
git checkout master
git merge "branchname"
git push -u origin master
