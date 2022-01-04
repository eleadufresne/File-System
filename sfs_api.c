// This is a C program that implements a simple file system (SFS) that can be
// mounted by the user under a single-level directory in the user’s machine (LINUX).
// Éléa Dufresne
// 2021-11-30

/* includes */
#include "sfs_api.h"
#include "disk_emu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* data structures (on-disk and in-memory) */
i_node_table_struct i_node_table; // i-Node table
directory_table_struct directory_table; // directory table

/* data structures (on-disk only) */
super_block_struct super_block; // super block
bit_map_struct bit_map; // free block bitmap (1 = free, 0 = allocated)

/* data structures (in-memory only)  */
FDT_struct FDT; // file descriptor table

/* global variables */
int current_file_index, dirs_iterated_over; // index of the current file in the directory, and number of directories parsed over (used in sfs_getnextfilename)

/* ( helper ) finds the next free block, return its index */
int next_free_block(void){
    // parse the bitmap
    for( int i=DATA_BLOCKS_ADDRESS; i < NUM_OF_BLOCKS; i ++){
        // if the current block is free, return its index
        if ( bit_map.is_free[i] ) return i;
    }
    // on failure return -1
    return -1;
}

/* ( helper ) finds the next free entry in the directory table */
int next_free_dir_entry(void){
    // parse every file
    for ( int i=0; i < MAX_FILES; i++ ) {
        // if this index is free
        if ( directory_table.directories[i].free ) return i;
    }
    // on failure, return -1
    return -1;
}

/* ( helper ) finds the index of the given file in the directory table */
int get_dir_index(const char *file){
    // number of directories
    int dirs = directory_table.num_of_dir;
    // parse every file
    for ( int i=0; i < MAX_FILES; i++ ) {
        // if this index is allocated
        if ( !(directory_table.directories[i].free) ) {
            dirs--;
            // if the file name matches the path, return the index
            if (!strcmp(file, directory_table.directories[i].filename)) {
                return i;
            }
        }
        // if no file in the directory matches the path
        if ( !dirs ) break;
    }
    // on failure, return -1
    return -1;
}

/* ( helper ) add a file to the FDT and return its index */
int create_FDT_entry(int i_node){
    // add the file corresponding to this i-Node number to the next free spot in the FDT
    for (int i=0; i<=MAX_FILES; i++){
        // if the current index is free, create the entry there
        if( FDT.file_descriptors[i].i_node_number == -1 ){
            // we set the pointers at the end of the file
            FDT.file_descriptors[i].i_node_number = i_node;
            FDT.file_descriptors[i].read_write_ptr = i_node_table.i_nodes[i_node].size;
            // open the file (set active)
            i_node_table.i_nodes[i_node].mode = ACTIVE;
            // update the number of opened files
            FDT.num_of_files++;
            // return the index
            return i;
        }
    }
    // on failure return -1
    return -1;
}

/* ( helper ) write the i-Node table to the disk */
void write_i_node( int index ){
    // buffer for the block
    i_node i_node_table_block[I_NODE_TABLE_BLOCKS*BLOCK_SIZE];
    // read the i-Node table from the disk to the buffer
    read_blocks(I_NODE_TABLE_ADDRESS, I_NODE_TABLE_BLOCKS, &i_node_table_block);
    // update the i-Node at the given index
    i_node_table_block[index].mode = i_node_table.i_nodes[index].mode;
    i_node_table_block[index].size = i_node_table.i_nodes[index].size;
    i_node_table_block[index].link_count = i_node_table.i_nodes[index].link_count;
    for( int j=0; j < NUM_OF_DIR_PTR; j++) i_node_table_block[index].direct_ptr[j] = i_node_table.i_nodes[index].direct_ptr[j];
    i_node_table_block[index].indirect_ptr= i_node_table.i_nodes[index].indirect_ptr;
    // write the updated table to memory
    write_blocks(I_NODE_TABLE_ADDRESS, I_NODE_TABLE_BLOCKS, &i_node_table_block);
}

/* ( helper ) read the i-Node table to that is on the disk */
void read_i_nodes( void ){
    // buffer for the block
    i_node i_node_table_block[I_NODE_TABLE_BLOCKS*BLOCK_SIZE];
    // read the i-Node table from the disk to the buffer
    read_blocks(I_NODE_TABLE_ADDRESS, I_NODE_TABLE_BLOCKS, &i_node_table_block);
    // copy the i-Node table we just read into the one in memory
    i_node_table.num_of_i_nodes = directory_table.num_of_dir;
    for( int i = 0; i < MAX_FILES; i++ ){
        i_node_table.i_nodes[i].mode = INACTIVE; // assume every file is closed
        i_node_table.i_nodes[i].size = i_node_table_block[i].size;
        i_node_table.i_nodes[i].link_count = i_node_table_block[i].link_count;
        for( int j=0; j<NUM_OF_DIR_PTR; j++) i_node_table.i_nodes[i].direct_ptr[j] = i_node_table_block[i].direct_ptr[j];
        i_node_table.i_nodes[i].indirect_ptr = i_node_table_block[i].indirect_ptr;
    }
}

/* create an instance of the simple file system */
void mksfs(int fresh){
    // close any open disk
    close_disk();

    // initialize empty data structures
    i_node_table.num_of_i_nodes = 0;
    directory_table.num_of_dir = 0;
    bit_map.size = 0;
    FDT.num_of_files = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        // initialize the empty file descriptor
        FDT.file_descriptors[i].i_node_number = -1;
        FDT.file_descriptors[i].read_write_ptr = 0;

        // initialize empty directory table
        directory_table.directories[i].free = 1;
        strcpy(directory_table.directories[i].filename, "\0");

        // initialize empty i-Node table
        i_node_table.i_nodes[i].mode = INACTIVE;
        i_node_table.i_nodes[i].size = 0;
        i_node_table.i_nodes[i].link_count = 0;
        for(int j = 0; j < NUM_OF_DIR_PTR; j++) i_node_table.i_nodes[i].direct_ptr[j] = -1;
        i_node_table.i_nodes[i].indirect_ptr = -1;
        i_node_table.i_nodes[i].size = 0;
    }

    // initialize the free block list
    for(int i=0; i <  NUM_OF_BLOCKS; i++ ) bit_map.is_free[i]=1;

    // in both cases we are pointing at the first file (skip the root)
    current_file_index = 1;
    dirs_iterated_over = 1;

    // if we need to create a new file system
    if ( fresh ) {
        // create a fresh file system
        init_fresh_disk("file_system.sfs", BLOCK_SIZE , NUM_OF_BLOCKS );

        // fill it with empty blocks
        void *empty_disk = malloc(BLOCK_SIZE * NUM_OF_BLOCKS );
        write_blocks(0,NUM_OF_BLOCKS, empty_disk);
        free( empty_disk );

        // write every i-Node to the disk
        for (int i = 0; i < MAX_FILES; i++) write_i_node(i);

        // initialize the super block
        strcpy(super_block.magic, MAGIC);
        super_block.block_size = BLOCK_SIZE;
        super_block.file_system_size = NUM_OF_BLOCKS;
        super_block.i_node_table_length = MAX_FILES;
        super_block.root = ROOT_DIRECTORY_ADDRESS;
        // write it to the disk and mark the block as allocated
        char super_blocks[BLOCK_SIZE] = {0};
        memcpy(super_blocks, &super_block, sizeof(super_block_struct));
        write_blocks(SUPER_BLOCK_ADDRESS, 1, &super_blocks);

        // initialize the i-Node for the root directory and write it to the disk
        i_node_table.i_nodes[0].mode= ROOT;
        i_node_table.i_nodes[0].size = 0;
        i_node_table.i_nodes[0].link_count = 1;
        i_node_table.i_nodes[0].direct_ptr[0] = ROOT_DIRECTORY_ADDRESS;
        i_node_table.num_of_i_nodes = 1;
        // write the root's i-Node to the disk
        write_i_node(0);

        // initialize the directory table (only the root so far) and write it to the disk
        directory_table.directories[0].free = 0;
        strcpy(directory_table.directories[0].filename, "~\0");
        directory_table.num_of_dir = 1;
        char directory_blocks[ROOT_DIRECTORY_BLOCKS*BLOCK_SIZE] = {0};
        memcpy(directory_blocks, &directory_table, sizeof(directory_table_struct));
        write_blocks(ROOT_DIRECTORY_ADDRESS, ROOT_DIRECTORY_BLOCKS, &directory_blocks);

        // flag the allocated blocks to the free bitmap
        for(int i=0; i < DATA_BLOCKS_ADDRESS; i++ ){
            bit_map.is_free[i]=0;
            bit_map.size++;
        }
        // write it to the disk
        char bit_map_blocks[FREE_BITMAP_BLOCKS*BLOCK_SIZE] = {0};
        memcpy(bit_map_blocks, &bit_map, sizeof(bit_map_struct));
        write_blocks(FREE_BITMAP_ADDRESS, FREE_BITMAP_BLOCKS, &bit_map_blocks );

    // if we are re-opening a previous file system
    } else {
        // re-open a previous file system
        init_disk("file_system.sfs", BLOCK_SIZE , NUM_OF_BLOCKS);

        // read the super block from the disk and initialize it
        //read_blocks(SUPER_BLOCK_ADDRESS, 1, &super_block);
        char super_blocks[BLOCK_SIZE] = {0};
        read_blocks(SUPER_BLOCK_ADDRESS, 1, &super_blocks);
        memcpy(&super_block, super_blocks, sizeof(super_block_struct));

        // read the bitmap from the disk
        //read_blocks(FREE_BITMAP_ADDRESS, FREE_BITMAP_BLOCKS, &bit_map);
        char bit_map_blocks[FREE_BITMAP_BLOCKS*BLOCK_SIZE] = {0};
        read_blocks(FREE_BITMAP_ADDRESS, FREE_BITMAP_BLOCKS, &bit_map_blocks);
        memcpy(&bit_map, bit_map_blocks, sizeof(bit_map_struct));

        // read the directory table from the disk
        //read_blocks(ROOT_DIRECTORY_ADDRESS, ROOT_DIRECTORY_BLOCKS, &directory_table);
        char directory_blocks[ROOT_DIRECTORY_BLOCKS*BLOCK_SIZE] = {0};
        read_blocks(ROOT_DIRECTORY_ADDRESS, ROOT_DIRECTORY_BLOCKS, &directory_blocks);
        memcpy(&directory_table, directory_blocks, sizeof(directory_table_struct));

        // read the i-Node table from the disk
        read_i_nodes();
    }
}

/* get the name of the next file in the directory */
int sfs_getnextfilename(char *fname){
    // get next file in the directory table
    while( directory_table.directories[current_file_index].free ) current_file_index++;
    ++dirs_iterated_over;
    // if there are no more directories to list return 0 and reset the variables
    if( dirs_iterated_over > directory_table.num_of_dir ) {
            dirs_iterated_over=1;
            current_file_index=1;
            return 0;
    }
    // copy the name of the file into fname
    strcpy(fname, directory_table.directories[current_file_index++].filename);
    // if there are still files to list, return 1
    return 1;
}

/* get the size of the specified file */
int sfs_getfilesize(const char *path){
    // get the index of the file in the directory table
    int index = get_dir_index(path);
    // on failure, return -1
    if ( index == -1 ) {
        fprintf(stderr,"Error, file %s does not exists.\n", path);
        return -1;
    }
    // on success, return its size (index of dir table <=> i-Node number)
    return i_node_table.i_nodes[index].size;
}

/* open the specified file in append mode, return the file descriptor
 * if the file does not exist, create a new file and sets its size to 0 */
int sfs_fopen(char *fname){
    // number of directories
    int dirs = directory_table.num_of_dir;
    // parse every file
    for ( int i=0; i < MAX_FILES; i++ ){
        // if this index is allocated
        if ( !( directory_table.directories[i].free ) ) {
            // if the file name matches the path
            if( ! strcmp(fname, directory_table.directories[i].filename ) ){
                // check if the file is already opened
                int is_active = i_node_table.i_nodes[i].mode;
                // if it is already opened, raise an error
                if ( is_active ) {
                    fprintf(stderr,"Error, file %s is already opened.\n", fname);
                    return -1;
                // otherwise, add it to the next available spot in the FDT and return its index
                } else return create_FDT_entry(i);
            }
        }
        // if no file in the directory matches the path, break the loop
        if ( !(--dirs) ) break;
    }
    // if we can't create a new file
    if( (directory_table.num_of_dir) > MAX_FILES  ){
        fprintf(stderr,"Error, new file could not be created : file system capacity exceeded.\n");
        return -1;
    }
    // if the file name is too long
    if( strlen( fname ) > MAX_FILENAME + MAX_FILE_EXTENSION + 2 ){
        fprintf(stderr,"Error, new file could not be created : file name is too long.\n");
        return -1;
    }
    // next free directory table index, and i-Node number
    int i_node_index= next_free_dir_entry();
    // raise an error if we don't find free entries
    if ( i_node_index == -1 ) {
        fprintf(stderr,"Error, new file could not be created.\n");
        return -1;
    }
    // update the directory table
    strcpy(directory_table.directories[i_node_index].filename, fname);
    directory_table.directories[i_node_index].free = 0;
    directory_table.num_of_dir++;
    // write the updated directory table to the disk
    char directory_blocks[ROOT_DIRECTORY_BLOCKS*BLOCK_SIZE] = {0};
    memcpy(directory_blocks, &directory_table, sizeof(directory_table_struct));
    write_blocks(ROOT_DIRECTORY_ADDRESS, ROOT_DIRECTORY_BLOCKS, (void *)directory_blocks);
    // create a new i-Node at the next available spot
    i_node_table.i_nodes[i_node_index].mode= ACTIVE;
    i_node_table.i_nodes[i_node_index].size= 0;
    i_node_table.i_nodes[i_node_index].link_count = 0;
    i_node_table.num_of_i_nodes++;
    // write the i-Node table to the disk
    write_i_node(i_node_index);
    // update the root's size, and cache it
    i_node_table.i_nodes[0].size ++;
    write_i_node(0);
    // add it to the next available spot in the FDT and return its index
    return create_FDT_entry(i_node_index);
}

/* close the specified file (remove the entry from the open file descriptor table) */
int sfs_fclose(int fileID) {
    // i-Node number
    int i_node = FDT.file_descriptors[fileID].i_node_number;
    // if the file ID is invalid
    if ( fileID < 0 || fileID >= MAX_FILES ) {
        fprintf(stderr,"Error, invalid file ID %d.\n", fileID);
        return -1;
    }
    // if there isn't an open file associated to this ID
    if ( !(FDT.num_of_files) || i_node == -1 ) {
        fprintf(stderr,"Error, no open file is associated with file ID %d.\n", fileID);
        return -1;
    }
    // if this file is opened, close it (set it as inactive) and write the i-Node to the disk
    i_node_table.i_nodes[i_node].mode = INACTIVE;
    write_i_node(i_node);
    // remove this file from the FDT
    FDT.file_descriptors[fileID].i_node_number = -1;
    FDT.file_descriptors[fileID].read_write_ptr = 0;
    FDT.num_of_files--;
    // on success, return 0
    return 0;
}

/* ( helper function for sfs_fwrite ) write to the block at the given address and return the number of bytes written */
int write_helper( int block_address, const char *write_buffer, char *block_buffer, int position_in_block, int remaining, int num_of_bytes_written){
    // load the current block
    read_blocks(block_address, 1, block_buffer);
    // we write as many bytes as we can
    int bytes_to_write =  MIN( (( !position_in_block ) ? BLOCK_SIZE : BLOCK_SIZE-position_in_block) , remaining);
    // append buf from where we need to write in the current block
    memcpy( block_buffer+position_in_block, write_buffer+num_of_bytes_written, bytes_to_write );
    // write the block to the disk
    write_blocks( block_address, 1, block_buffer);
    // free the buffer
    free( block_buffer );
    // return the number of bytes we just wrote
    return bytes_to_write;
}

/* write buffer characters onto an already opened file on the disk and return the number of bytes written */
int sfs_fwrite(int fileID, const char *buf, int length) {
    // number of bytes that have been read so far
    int num_of_bytes_written = 0;
    // if the file ID is invalid
    if ( fileID < 0 || fileID >= MAX_FILES ) {
        fprintf(stderr,"Error, invalid file ID %d.\n", fileID);
        return 0;
    }
    // if the length is invalid
    if ( length < 0 ) {
        fprintf(stderr,"Error, invalid string length %d\n", length);
        return 0;
    }
    // i-Node number
    int i_node = FDT.file_descriptors[fileID].i_node_number;
    // if there isn't an open file associated to this ID
    if ( !(FDT.num_of_files) || i_node == -1 ) {
        fprintf(stderr,"Error, no open file is associated with file ID %d.\n", fileID);
        return 0;
    }
    // current size, position in file we write from, we write to
    int curr_size = i_node_table.i_nodes[i_node].size;
    int write_from = FDT.file_descriptors[fileID].read_write_ptr;
    int write_to = write_from + length;
    // if we are trying to write past the maximum file size
    if( write_to > MAX_FILE_SIZE ){
        fprintf(stderr, "Error, seeking to write past the maximum file size.\n");
        return 0;
    }
    // update the file's size if we need to increase it
    i_node_table.i_nodes[i_node].size = MAX(write_to, curr_size);
    // current number of blocks allocated to this i_Node, and number needed
    int curr_num_of_blocks = i_node_table.i_nodes[i_node].link_count;
    // pointer position relative to the current block
    int curr_block_index = write_from / BLOCK_SIZE  ;
    int position_in_block =  write_from % BLOCK_SIZE ;
    // if we need to write extra blocks but can't
    if ( bit_map.size + CEILING (length - position_in_block , BLOCK_SIZE ) > NUM_OF_BLOCKS ) {
        fprintf(stderr, "Error, seeking to write past maximal capacity of the file system.\n");
        return 0;
    }
    // current block info
    int block_address;
    // while do not need extra blocks, write from the pointer ( we overwrite what's after if it's not at the end )
    if( curr_block_index < curr_num_of_blocks ) {
        // while/if the current block index is within the direct pointers scope
        while ( curr_block_index < curr_num_of_blocks && num_of_bytes_written < length && curr_block_index < NUM_OF_DIR_PTR) {
            // address of the current block
            block_address = i_node_table.i_nodes[i_node].direct_ptr[curr_block_index++];
            // write to the block and update the number of bytes written
            num_of_bytes_written += write_helper(block_address, buf, malloc(BLOCK_SIZE), position_in_block,
                                                 length - num_of_bytes_written, num_of_bytes_written);
            // new position in block is at the start
            position_in_block = 0;
        }
        // while we need to write, and the current block index is in the indirect pointer scope
        if (curr_block_index < curr_num_of_blocks && num_of_bytes_written < length) {
            // load the block of addresses pointed by the indirect pointer
            int addresses[BLOCK_SIZE];
            block_address = i_node_table.i_nodes[i_node].indirect_ptr;
            read_blocks(block_address, 1, addresses);
            // for every address it holds
            for (int i = 0; i < (curr_num_of_blocks - NUM_OF_DIR_PTR); i++) {
                // address of the current block
                block_address = addresses[i];
                // write to the block and update the number of bytes written
                num_of_bytes_written += write_helper(block_address, buf, malloc(BLOCK_SIZE), position_in_block,
                                                     length - num_of_bytes_written, num_of_bytes_written);
                // new position in block is at the start
                position_in_block = 0;
                curr_block_index++;
                // if we are done, break the loop
                if (num_of_bytes_written >= length) break;
            }
        }
    }
    // if we need to write extra blocks ( we assume we have already written to the available blocks )
    while ( num_of_bytes_written < length ){
        // get next available block
        block_address = next_free_block();
        // set as allocated in the free bitmap
        bit_map.is_free[block_address] = 0;
        bit_map.size ++;
        // write to the block and update the number of bytes written
        num_of_bytes_written += write_helper(block_address, buf, malloc(BLOCK_SIZE), position_in_block,
                                             length - num_of_bytes_written, num_of_bytes_written);
        // new position in block is at the start
        position_in_block = 0;
        // if we are in the indirect pointer scope
        if( curr_block_index >= NUM_OF_DIR_PTR ){
            // if we need to create a block of addresses
            if ( i_node_table.i_nodes[i_node].indirect_ptr == -1 ) {
                // get next available block
                int ptr_block_address = next_free_block();
                // set as allocated in the free bitmap
                bit_map.is_free[ptr_block_address] = 0;
                bit_map.size++;
                // update the indirect pointer
                i_node_table.i_nodes[i_node].indirect_ptr = ptr_block_address;
                // write the indirect pointer block (empty for now)
                void *empty_block = malloc(BLOCK_SIZE);
                write_blocks(ptr_block_address, 1, empty_block);
                free( empty_block );
            }
            // load the block of addresses pointed by the indirect pointer
            int addresses[BLOCK_SIZE];
            int ptr_block_address = i_node_table.i_nodes[i_node].indirect_ptr;
            write_blocks(ptr_block_address, 1, addresses);
            // append the current block number at the end
            addresses[curr_block_index - NUM_OF_DIR_PTR] = block_address;
            // write the updated block to the disk
            write_blocks(ptr_block_address, 1, addresses);
        } else { // if the current block index is within the direct pointers scope
            // update the appropriate direct pointer
            i_node_table.i_nodes[i_node].direct_ptr[curr_block_index] = block_address;
        }
        // update the i-Node link count and index
        curr_block_index = ++(i_node_table.i_nodes[i_node].link_count);
    }
    // write the bitmap to the disk
    char bit_map_blocks[FREE_BITMAP_BLOCKS*BLOCK_SIZE] = {0};
    memcpy(bit_map_blocks,&bit_map, sizeof(bit_map_struct));
    write_blocks(FREE_BITMAP_ADDRESS, FREE_BITMAP_BLOCKS, &bit_map_blocks );
    // update the write pointer
    FDT.file_descriptors[fileID].read_write_ptr = write_to;
    // return the number of bytes written
    return num_of_bytes_written;
}

/* read characters from the disk into the buffer */
int sfs_fread(int fileID, char *buf, int length){
    int num_of_bytes_read = 0;
    // if the file ID is invalid
    if ( fileID < 0 || fileID >= MAX_FILES ) {
        fprintf(stderr,"Error, invalid file ID %d.\n", fileID);
        return 0;
    }
    // i-Node number
    int i_node = FDT.file_descriptors[fileID].i_node_number;
    // if there isn't an open file associated to this ID
    if ( !(FDT.num_of_files) || i_node == -1 ) {
        fprintf(stderr,"Error, no open file is associated with file ID %d.\n", fileID);
        return 0;
    }
    // if the length is invalid
    if ( length < 0 ) {
        fprintf(stderr,"Error, trying to read negative length.\n");
        return 0;
    }
    // interval in which we read
    int read_from = FDT.file_descriptors[fileID].read_write_ptr;
    int end_of_file = i_node_table.i_nodes[i_node].size;
    /* int read_to = MIN( end_of_file , read_from + length); */
    int reading_length = MIN( end_of_file - read_from , length );
    // if the pointer is at the end of the file, nothing to read
    if( read_from == end_of_file ) {
        fprintf(stderr,"Error, pointer is already at the end of the file.\n");
        return 0;
    }
    // current block index and position of the pointer in it
    int curr_block_index =  read_from / BLOCK_SIZE ;
    int position_in_block = read_from % BLOCK_SIZE ;
    // variables
    char *block_buffer;
    int bytes_to_read, block_address, *addresses;
    // while we still need to load some blocks, and we don't are within direct pointers scope
    while( (num_of_bytes_read < reading_length) && (curr_block_index < NUM_OF_DIR_PTR) ) {
        // load the desired blocks
        block_buffer = malloc(BLOCK_SIZE);
        block_address = i_node_table.i_nodes[i_node].direct_ptr[curr_block_index++];
        read_blocks( block_address, 1, block_buffer);
        // we read as many bytes as we can
        bytes_to_read =  MIN( (( !position_in_block ) ? BLOCK_SIZE : BLOCK_SIZE-position_in_block) , (reading_length-num_of_bytes_read) );
        // save the content from the read pointer into buf
        memcpy( buf+num_of_bytes_read , block_buffer+position_in_block, bytes_to_read);
        // free the buffer, update the variables
        position_in_block = 0;
        num_of_bytes_read += bytes_to_read;
        free(block_buffer);
    }
    // while we still need to load some blocks, and we are NOT within direct pointers scope
    if( num_of_bytes_read < reading_length ) {
        // load the block of addresses pointed by the indirect pointer
        addresses = malloc(BLOCK_SIZE);
        block_address = i_node_table.i_nodes[i_node].indirect_ptr;
        read_blocks(block_address, 1, addresses);
        // for every address it holds
        while ( num_of_bytes_read < reading_length ) {
            // address of the current block
            block_address = addresses[curr_block_index++ - NUM_OF_DIR_PTR];
            // load the desired blocks
            block_buffer = malloc(BLOCK_SIZE);
            read_blocks( block_address, 1, block_buffer);
            // we read as many bytes as we can
            bytes_to_read =  MIN( (( !position_in_block ) ? BLOCK_SIZE : BLOCK_SIZE-position_in_block) , (reading_length-num_of_bytes_read) );
            // save the content from the read pointer into buf
            memcpy( buf+num_of_bytes_read , block_buffer+position_in_block, bytes_to_read);
            // free the buffer, update the variables
            position_in_block = 0;
            num_of_bytes_read += bytes_to_read;
            free(block_buffer);
        }
        // free the buffer
        free(addresses);
    }
    // update the read pointer
    FDT.file_descriptors[fileID].read_write_ptr = read_from + num_of_bytes_read;
    // return the number of bytes read
    return num_of_bytes_read;
}

/* seek (move the read/write pointer) to the specified location */
int sfs_fseek(int fileID, int location){
    // i-Node number
    int i_node = FDT.file_descriptors[fileID].i_node_number;
    // if the file ID is invalid
    if ( fileID < 0 || fileID >= MAX_FILES ) {
        fprintf(stderr,"Error, invalid file ID %d.\n", fileID);
        return -1;
    }
    // if there isn't an open file associated to this ID
    if (  !(FDT.num_of_files) || i_node == -1 ) {
        fprintf(stderr,"Error, no open file is associated with file ID %d.\n", fileID);
        return -1;
    }
    // if the location exceeds the boundary
    if ( location < 0 || i_node_table.i_nodes[i_node].size < location ) {
        fprintf(stderr,"Error, location exceeds boundaries of file %d.\n", fileID);
        FDT.file_descriptors[fileID].read_write_ptr = i_node_table.i_nodes[FDT.file_descriptors[fileID].i_node_number].size;
        return -1;
    }
    // if we are trying to seek past the maximum file size
    if( location > MAX_FILE_SIZE ){
        fprintf(stderr, "Error, seeking location past the maximum file size.\n");
        return 0;
    }
    // otherwise, move the pointers to the location
    FDT.file_descriptors[fileID].read_write_ptr = location;
    // on success, return 0
    return 0;
}

/* remove a file from the file system (release the data blocks, i-Node, directory entry, etc.) */
int sfs_remove(char *file){
    // index of the file in the directory table (i.e. its i-Node number)
    int i_node_index = get_dir_index(file);
    // if no such file exists, return -1
    if ( i_node_index == -1 ) {
        fprintf(stderr,"Error, file %s does not exists.\n", file);
        return -1;
    }
    // if the file is opened, we need to close it first
    if ( i_node_table.i_nodes[i_node_index].mode ) {
        fprintf(stderr,"Error, file must be closed before being removed.\n");
        return -1;
    }
    // otherwise, release the data blocks used by the file
    int block_address;
    int num_of_blocks = i_node_table.i_nodes[i_node_index].link_count;
    // free all the blocks that it uses
    for( int i=0; i < NUM_OF_DIR_PTR; i++){
        if ( i >= num_of_blocks ) break;
        // block number and address
        block_address = i_node_table.i_nodes[i_node_index].direct_ptr[i];
        // free it from the bitmap
        bit_map.is_free[block_address] = 1;
        bit_map.size--;
        // update the i-Node direct pointer
        i_node_table.i_nodes[i_node_index].direct_ptr[i] = -1;
        // overwrite the current block
        void *empty_block = malloc(BLOCK_SIZE);
        write_blocks(block_address, 1, empty_block);
        free( empty_block );
    }
    // free the blocks pointed by the indirect pointer if needed
    if ( num_of_blocks > NUM_OF_DIR_PTR  ){
        // block of indirect pointer number and address
        int ptr_block_address = i_node_table.i_nodes[i_node_index].indirect_ptr;
        // load the adresses
        int addresses[BLOCK_SIZE];
        read_blocks(ptr_block_address, 1, addresses);
        for( int i=0; i < (num_of_blocks-NUM_OF_DIR_PTR); i++){
            // block number and address
            block_address = addresses[i];
            // free it from the bitmap
            bit_map.is_free[block_address] = 1;
            bit_map.size--;
            // overwrite the current block
            void *empty_block = malloc(BLOCK_SIZE);
            write_blocks(block_address, 1, empty_block);
            free( empty_block );
        }
        // update the i-Node indirect pointer
        i_node_table.i_nodes[i_node_index].indirect_ptr = -1;
        // overwrite the pointer block
        void *empty_block = malloc(BLOCK_SIZE);
        write_blocks(ptr_block_address, 1, empty_block);
        free( empty_block );
        // free the pointer block from the bitmap
        bit_map.is_free[ptr_block_address] = 1;
        bit_map.size--;
    }
    // free the i-Node
    i_node_table.i_nodes[i_node_index].mode = INACTIVE;
    i_node_table.i_nodes[i_node_index].size = 0;
    i_node_table.i_nodes[i_node_index].link_count = 0;
    i_node_table.i_nodes[i_node_index].indirect_ptr = -1;
    i_node_table.num_of_i_nodes--;
    // write the i-Node table to the disk
    write_i_node(i_node_index);
    // remove the file from the directory entry
    directory_table.directories[i_node_index].free = 1;
    strcpy(directory_table.directories[i_node_index].filename, "\0");
    directory_table.num_of_dir--;
    // write the updated directory table to the disk
    char directory_blocks[ROOT_DIRECTORY_BLOCKS*BLOCK_SIZE] = {0};
    memcpy(directory_blocks, &directory_table, sizeof(directory_table_struct));
    write_blocks(ROOT_DIRECTORY_ADDRESS, ROOT_DIRECTORY_BLOCKS, &directory_blocks);
    // write the updated free block bit map to the disk
    char bit_map_blocks[FREE_BITMAP_BLOCKS*BLOCK_SIZE] = {0};
    memcpy(bit_map_blocks,&bit_map, sizeof(bit_map_struct));
    write_blocks(FREE_BITMAP_ADDRESS, FREE_BITMAP_BLOCKS, &bit_map_blocks );
    // on success, return 0
    return 0;
}
