#ifndef SFS_API_H
#define SFS_API_H

/* mathematical functions */
#define MIN(a, b)                          ( ( (a) < (b) ) ? (a) : (b) )    // gives the minimum value between a and b
#define MAX(a, b)                          ( ( (a) > (b) ) ? (a) : (b) )    // gives the maximum value between a and b
#define CEILING(a, b)                      ( ( (a) + (b) - 1 ) / (b) )      // gives the ceiling of a/b

/* constants */
#define MAGIC                              "0xACBD0005"                     // magic number
#define BLOCK_SIZE                         1024                             // size of a data block

#define NUM_OF_DIR_PTR                     12                               // number of direct pointers per i-Node
#define PTR_SIZE                           sizeof(int)                      // size of a pointer ( it's an integer )

#define INACTIVE                           0                                // file is open
#define ACTIVE                             1                                // file is closed
#define ROOT                               2                                // root directory

#define MAX_FILENAME                       16                               // maximum length for a file name
#define MAX_FILE_EXTENSION                 3                                // maximum length for a file extension
#define MAX_FILES                          500                              // maximum number of files the system can hold ( arbitrary )
#define MAX_FILE_SIZE                      ( ( BLOCK_SIZE * NUM_OF_DIR_PTR ) + ( BLOCK_SIZE * ( BLOCK_SIZE / PTR_SIZE ) ) ) // maximum size a file can have

#define NUM_OF_BLOCKS                      CEILING(  MAX_FILES * MAX_FILE_SIZE , BLOCK_SIZE )   // maximum number of data blocks the file system can hold

#define ROOT_DIRECTORY_BLOCKS              CEILING(  sizeof( directory_table_struct ) , BLOCK_SIZE) // number of blocks needed to hold the directory table
#define I_NODE_TABLE_BLOCKS                CEILING(  sizeof ( i_node_table_struct ) , BLOCK_SIZE )  // number of blocks needed to hold the i-Node table
#define FREE_BITMAP_BLOCKS                 CEILING(  sizeof( bit_map_struct ) , BLOCK_SIZE )  // number of blocks needed to hold the free blocks bitmap

#define SUPER_BLOCK_ADDRESS                0                                                    // address of the super block
#define I_NODE_TABLE_ADDRESS               1                                                    // address of the i-Node table
#define FREE_BITMAP_ADDRESS                ( I_NODE_TABLE_ADDRESS + I_NODE_TABLE_BLOCKS )       // address of the free bitmap
#define ROOT_DIRECTORY_ADDRESS             ( FREE_BITMAP_ADDRESS + FREE_BITMAP_BLOCKS )         // address of the directory table
#define DATA_BLOCKS_ADDRESS                ( ROOT_DIRECTORY_ADDRESS + ROOT_DIRECTORY_BLOCKS )   // minimum address of the data blocks (to hold the files' content)


/* data structures */
// file descriptor table
typedef struct {
    int i_node_number; // -1 if this entry corresponds to no open file
    int read_write_ptr; // position of the pointer in the file
} file_descriptor_entry;
typedef struct {
    file_descriptor_entry file_descriptors[ MAX_FILES ]; // file descriptor table
    int num_of_files; // current number of opened files
} FDT_struct;

// directory table
typedef struct {
    int free; // 1 = free, 0 = allocated
    char filename[ MAX_FILENAME + MAX_FILE_EXTENSION + 2 ]; // +1 dot +1 null terminated
} directory_entry;
typedef struct {
    directory_entry directories[ MAX_FILES ]; // directory table (the index corresponds to the i-Node number)
    int num_of_dir; // current number of directories
} directory_table_struct;

// super block
typedef struct {
    char magic[16]; // to recognize whether the disk file is compatible
    int block_size; // size of the data blocks
    int file_system_size; // file system size and i-node table length
    int i_node_table_length; // maximum number of i-Nodes
    int root; // address of the root directory
} super_block_struct;

// i-Node
typedef struct {
    int mode; //  ACTIVE or INACTIVE
    int size; // size of the file associated with this i-Node
    int link_count; // number of (indirect and direct) pointers
    int direct_ptr[ NUM_OF_DIR_PTR ]; // blocks (number) this i-Node needs
    int indirect_ptr; // other dirs after this one
} i_node;
typedef struct {
    i_node i_nodes[ MAX_FILES ]; // i-Node table
    int num_of_i_nodes; // number of allocated i-Nodes (prevents from parsing the whole table)
} i_node_table_struct;

// bitmap to keep track of free/allocated space
typedef struct {
    int is_free[ NUM_OF_BLOCKS ]; // 1 = free, 0 = allocated
    int size; // number of allocated blocks
} bit_map_struct;

/* helper functions */
int next_free_block(void);
int next_free_dir_entry(void);
int get_dir_index(const char*);
int create_FDT_entry(int);
void write_i_node(int);
void read_i_nodes(void);

/* API functions */
void mksfs(int);
int sfs_getnextfilename(char*);
int sfs_getfilesize(const char*);
int sfs_fopen(char*);
int sfs_fclose(int);
int sfs_fwrite(int, const char*, int);
int sfs_fread(int, char*, int);
int sfs_fseek(int, int);
int sfs_remove(char*);

#endif
