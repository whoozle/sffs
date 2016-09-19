#YFFS, version 1.1
###CS307: Brian VerVaet, Max Molnar, Sam Spencer, Wyatt Dahlenburg, Zachary Kent

Many types of devices use Flash for persistent storage. In situations where power fluctuates (batteries running out, power
or useless. YFFS is a filesystem designed to avoid these hazards and be used as a secure, dependable storage medium on any flash
device.  YFFS provides stable storage despite power fluctuations by implementing a transaction-based file system. This type of 
file system is designed to leave the structure of directory, files, and metadata in a sound state. Yffs accomplishes this by 
finalizing metadata with a one-bit switch, ensuring it won't lose anything. While there are transaction-based file systems that 
accomplish the task of flash stability, no such file system exists today that also provides an interface for user-defined 
encryption.  

