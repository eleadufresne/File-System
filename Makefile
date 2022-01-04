CFLAGS = -c -g -ansi -pedantic -Wall -std=gnu99 `pkg-config fuse --cflags --libs`

LDFLAGS = `pkg-config fuse --cflags --libs`

# make executables for every test, then the fuse mounts
SOURCES_TEST_0 = disk_emu.c sfs_api.c sfs_test0.c sfs_api.h
SOURCES_TEST_1 = disk_emu.c sfs_api.c sfs_test1.c sfs_api.h
SOURCES_TEST_2 = disk_emu.c sfs_api.c sfs_test2.c sfs_api.h
SOURCES_FUSE_OLD = disk_emu.c sfs_api.c fuse_wrap_old.c sfs_api.h
SOURCES_FUSE_NEW = disk_emu.c sfs_api.c fuse_wrap_new.c sfs_api.h

OBJECTS_TEST_0 = $(SOURCES_TEST_0:.c=.o)
OBJECTS_TEST_1 = $(SOURCES_TEST_1:.c=.o)
OBJECTS_TEST_2 = $(SOURCES_TEST_2:.c=.o)
OBJECTS_FUSE_OLD = $(SOURCES_FUSE_OLD:.c=.o)
OBJECTS_FUSE_NEW = $(SOURCES_FUSE_NEW:.c=.o)

EXECUTABLE_TEST_0 = sfs_test0_Elea_Dufresne
EXECUTABLE_TEST_1 = sfs_test1_Elea_Dufresne
EXECUTABLE_TEST_2 = sfs_test2_Elea_Dufresne
EXECUTABLE_FUSE_OLD = sfs_old_file_Elea_Dufresne
EXECUTABLE_FUSE_NEW = sfs_new_file_Elea_Dufresne

# all the programs
all : test0 test1 test2 fuse_old fuse_new

# test 0
test0: $(SOURCES_TEST_0) $(HEADERS) $(EXECUTABLE_TEST_0)
$(EXECUTABLE_TEST_0) : $(OBJECTS_TEST_0)
	gcc $(OBJECTS_TEST_0) $(LDFLAGS) -o $@

# test 1
test1: $(SOURCES_TEST_1) $(HEADERS) $(EXECUTABLE_TEST_1)
$(EXECUTABLE_TEST_1) : $(OBJECTS_TEST_1)
	gcc $(OBJECTS_TEST_1) $(LDFLAGS) -o $@

# test 2
test2: $(SOURCES_TEST_2) $(HEADERS) $(EXECUTABLE_TEST_2)
$(EXECUTABLE_TEST_2) : $(OBJECTS_TEST_2)
	gcc $(OBJECTS_TEST_2) $(LDFLAGS) -o $@

# fuse wrapper for mounting a new file system
fuse_old: $(SOURCES_FUSE_OLD) $(HEADERS) $(EXECUTABLE_FUSE_OLD)
$(EXECUTABLE_FUSE_OLD) : $(OBJECTS_FUSE_OLD)
	gcc $(OBJECTS_FUSE_OLD) $(LDFLAGS) -o $@

# fuse wrapper for mounting a new file system
fuse_new: $(SOURCES_FUSE_NEW) $(HEADERS) $(EXECUTABLE_FUSE_NEW)
$(EXECUTABLE_FUSE_NEW) : $(OBJECTS_FUSE_NEW)
	gcc $(OBJECTS_FUSE_NEW) $(LDFLAGS) -o $@

# compile
.c.o:
	gcc $(CFLAGS) $< -o $@

# clean all the executables
clean:
	rm -rf *.o *~ $(EXECUTABLE_TEST_0) $(EXECUTABLE_TEST_1) $(EXECUTABLE_TEST_2) $(EXECUTABLE_FUSE_OLD) $(EXECUTABLE_FUSE_NEW)