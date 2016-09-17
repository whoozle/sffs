# cs307





##Simple Fault-tolerant File System, version 1.1

This tiny filesystem was designed to be the most safe in the world of SPI flash 
filesystems. Writing into such flash splitted into single-byte commands and 
could be interrupted by power failure or crash and therefore corrupt your data.
Sffs finalizes metadata with one-bit switch and won't lose anything.

Also this filesystem will protect you from flash wearing. It saves timestamps
of writes and always rotate files around the flash, so you never got the same
file written again and again on the same place.

