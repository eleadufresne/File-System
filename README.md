## Overview
This repository implements a simple file system (SFS) that can be easily mounted by the user under a directory on their Linux machine. The file system has several limitations like restricted filename lengths, no user concept, no protection among files and no support for concurrent access. The CCdisk is created as a file on the actual file system and is divided into sectors of fixed size. The on-disk data structures of the file system include a super block, the root directory, free block list, and i-Node table. The super block sets out the file system's geometry and is also the first block in SFS. A file or directory in SFS is defined by an i-Node, which is pointed to by the super block. The i-Node structure is simplified and does not have the double and triple indirect pointers, only direct and single indirect pointers.

## Executables

The Makefile has five configurations. When compiling the code with the command ``make``, we get:

1. sfs_test0
2. sfs_test1
3. sfs_test2
4. sfs_old_file
5. sfs_new_file
   
Each one represents a different test or the fuse wrappers.

## TESTS 0-2 

Run the commands ``sfs_test0``, ``sfs_test1``, and 
``sfs_test2`` for the results of tests 0, 1, and 2 respectively. 
Changes have been made to the variables reflecting the specifications to 
match those of my own file system.

## FUSE 

The file system is ``file_system.sfs``. To mount an existing file system run the command ``./sfs_old_file
mount`` and for a fresh one, ``./sfs_new_file mount``. This is
assuming the directory used for mounting is "mount" but any other folder 
should work!
