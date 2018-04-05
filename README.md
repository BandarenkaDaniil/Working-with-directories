# Working with files and directories in Linux

**Operating systems & System Programming, Laboratory work #2**

## Task
Find matching files in two specified directories(1st and 2nd command line arguments) and all their subdirectories.
Output absolute path, size in bytes, last modification time, permissions, inode number to console and file (3rd command line argument). 

## Usage 
To compile all files use (in terminal):
  ```
  make
  ```
Running program:
  ```
  filescmp [first_dir] [second_dir] [output_file]
  ```
  
> **Note:** **make** creates object files and doesn't delete them. Call **make clean** to delete these files.

## Description 
In this program I use my module providing handling multiple C-string (file pathes in this case) using singly linked list. The program reads all the regular files from specified directories and saves pathes to them in lists. After that it moves through the lists, comparing all files.
