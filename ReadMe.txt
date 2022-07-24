Disk Simulator
Authored by Yazan Daefi
323139923

==Description==

The program is a simulation of processor approaches to the Hard Disk, done using C++, contains writting to and reading from a file that describes the disk.
The disk works with index allocation method to save the data. Receives an action by the user gives him ability to choose what he want do (create a new file, write\read, close\open\delete file).
Then the program works according to the space in the disk (known by the Bit vector) and other special cases to make sure that the action could be done.

Program DATABASE:

1.fsFile - a class that containes information about the file (file size, index block, number of blocks in use and file id).
2.FileDescriptor – a class that contains a description for each file (file name, if the file is opened, and a pointer to the fsFIle).
3.fsDisk – a class that containes information about the disk (pointer to the disk file and other parameters and data structures).
4.DISK_SIM_FILE - file that describes the disk.

functions:
Eight main functions:
1.listAll - prints two things: 1) information about the files that exists on the disk. 2) The disk's content.
2.fsFormat - formates the disk (initializes all the data structures of the disk and clears the disk).
3.CreateFile - adds a new file to the disk's data base, by initialize a new file descriptor and fsFile.
4.OpenFile - opens a file to get it ready to use by adding the file descriptor to the OpenFileDescriptors map.
5.CLoseFile - closes the file by remove it from the OpenFileDescriptors map.
6.WriteToFile - writes to a given fd of a file, checks the maximum file size, disk size, creating new blocks to write and more.
7.DelFile - deletes a file by a given file name by remove it from the database of the disk and deletes all it's data on the disk.
8.ReadFromFile - reads a given length of chars from from a given file fd on the disk.

six help functions:
1.createID - finds the first available fd to give it to a new file.
2.getEmptyBlock - finds the first empty block from the bit vector.
3.addBlockToFile - adds a new block number on the index block for a given file.
4.isOpened - checks if a given fd of a file is existd in the openFileDescriptors.
5.isExists - checks if a given file name is exists in the MainDir vector.
6.deleteBlock -  deletes a given block number by writting '\0' in.
                          
==Program Files==
sim_disk.cpp : contains all the classes and the functions with the initial main.

==How to compile?==
compile: g++ sim_disk.cpp -o final
run: ./final

==Input:==
According to the main function:
numbers from 0 to 8 that describes an action to do on the disk.
fds or files name according to the action.

==Output:==
Disk's content and information.
