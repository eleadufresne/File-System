----- EXECUTABLES -----------------------------------------------------------

The Makefile creates 5 executables when running the command "make" :
	1. sfs_test0_Elea_Dufresne
	2. sfs_test1_Elea_Dufresne
	3. sfs_test2_Elea_Dufresne
	4. sfs_old_file_Elea_Dufresne
	5. sfs_new_file_Elea_Dufresne
Each one is for a different test or the fuse wrappers.

----- TESTS 0-2 -------------------------------------------------------------

Run the commands "sfs_test0_Elea_Dufresne", "sfs_test1_Elea_Dufresne", and 
"sfs_test2_Elea_Dufresne" for the results of tests 0, 1, and 2 respectively. 
Changes have been made to the variables reflecting the specifications to 
match those of my own file system. My program should pass every test.

----- FUSE ------------------------------------------------------------------

This part should work as well, the file system is "file_system.sfs". To
mount an existing file system run the command "./sfs_old_file_Elea_Dufresne
mount" and for a fresh one, "./sfs_new_file_Elea_Dufresne mount". This is
assuming the directory used for mounting is "mount" but any other folder 
should work! 
