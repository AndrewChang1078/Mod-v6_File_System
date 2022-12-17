/*
Operating System(CS 5348) Project

Chih-An Chang  - CXC210017

open:
 open Filename - opens the file Filename and returns an file identifier. If a
file is not present it will create a new one.

 Example: open a.txt

Initfs:
 Initfs Blocknumber Inodenumber - Filesystem initialised with Blocknumber number
of blocks, Inodenumber of inodes.

 Example: initfs 300 10

 Quit:
 q - Close the file and close the file system

 Example: q

 Print:
 print Blocknumber - print the contents of the Blocknumber to command window.

 Example: print 30

 cpin externalfile v6-file - Creat a new file called v6-file in the v6 file system and fill the contents of the newly created file with the contents of the externalfile. The file externalfile is a file in the native unix machine, the system where your program is being run.
 
 cpout v6-file externalfile - If the v6-file exists, create externalfile and make the externalfile's contents equal to v6-file.
 
 rm v6-file - Delete the file v6-file from the v6 file system. Remove all the data blocks of the file, free the i-node and remove the directory entry.
 
 mkdir v6dir - create the v6dir. It should have two entries . 
 
 cd v6dir - change working directory of the v6 file system to the v6dir
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>

#define MAX_INPUT_STRING_SIZE 200
#define INODE_SIZE 64
#define I_SIZE 200
#define inode_alloc 0100000        
#define pfile 000000  
#define BLOCK_SIZE 1024
#define FREE_SIZE 251
#define INODE_ADDR_SIZE 9

/* i-node flags*/
#define FLAG_INODE_ALLOCATED 0x8000
#define FLAG_TYPE_BLOCK_SPECIAL_FILE 0x6000
#define FLAG_TYPE_DIRECTORY 0x4000
#define FLAG_TYPE_CHAR_SPECIAL_FILE 0x2000
#define FLAG_TYPE_PLAIN_FILE 0x0
#define FLAG_SIZE_SMALL 0x0
#define FLAG_SIZE_MEDIUM 0x800
#define FLAG_SIZE_LONG 0x1000
#define FLAG_SIZE_SUPER_LONG 0x1800
#define FLAG_DEFAULT_PERMIT 0x777 //rwx for world

typedef struct {
    int isize;
    int fsize;
    int nfree;
    unsigned int free[FREE_SIZE];
    int flock;
    int ilock;
    unsigned int fmod;
    unsigned int time;
} superblock_type; // Block size is 1024 Bytes; only 1023 Bytes are used

typedef struct {
    unsigned short flags;
    unsigned short nlinks;
    unsigned int uid;
    unsigned int gid;
    unsigned int size0;
    unsigned int size1;
    unsigned int addr[INODE_ADDR_SIZE];
    unsigned int actime;
    unsigned int modtime;
} inode_type; //64 Bytes in size

typedef struct {
    unsigned int inode;
    unsigned char filename[28];
} dir_type;//32 Bytes long

int fd = -1;
int dst_fd = -1;
int rootfd;

superblock_type sb;
inode_type root_inode;
unsigned int empty_space[BLOCK_SIZE / sizeof(unsigned int)];
unsigned int cur_inode = 1;    //Initialized to root i-node
bool *inode_free;

int add_block_to_free_list(int);
int openfs(char*);
int initfs(int, int);
int q(void);
void init_root(void);
void print_block(int);
void cpout(char*, char*);
unsigned int get_file_inode(char*);
int copy_small_file(inode_type, char*);
int copy_medium_file(inode_type, char*);
int copy_long_file(inode_type, char*);
int copy_super_long_file(inode_type, char*);
unsigned int cd(char*);
void cpin(const char* ,const char* );
unsigned short allocateinode();
void smallfile(const char *pathname1 , const char *pathname2 , int blocks_allocated);
void rm(char* );
void add_block_to_free_list(int ,  unsigned int *);
inode_type read_inode(int );
void update_rootdir(const char *pathname , unsigned short inode_number);
void create_directory(char*);//mkdir
int get_free_inode(void);
int request_block_from_free_list(void);

using namespace std;

int main () {
    cout << "Welcome to CS 5348 Project 2" << endl;

    while (1) {
        char user_input[MAX_INPUT_STRING_SIZE];
        char *command = NULL,*p1 = NULL, *p2 = NULL;

        cout << endl << "Please enter command [Valid commands:open filename,initfs n1 n2,q,print BlockNumber,cpout,cpin,cd,rm,mkdir]:" << endl;
        cin.getline(user_input, MAX_INPUT_STRING_SIZE);
        command = strtok(user_input, " ");
        if (strcmp(command, "open") == 0){
            p1 = strtok(NULL, " ");
            if (!p1) {
                cout << "Please provide a file name parameter (ex. openfile_name)" << endl;
            } else {
                openfs(p1);
            }
        } else if (strcmp(command, "initfs") == 0) {
            p1 = strtok(NULL, " ");
            p2 = strtok(NULL, " ");
            if (!p1 || !p2) {
                cout << "Please provide block number and inode number" << endl;
            } else {
                initfs(atoi(p1), atoi(p2));
            }
        } else if (strcmp(command, "q") == 0) {
            if (fd != -1) {
                int ret = q();
                if (ret != 0) {
                    cout << "Close file failed, error code = " << ret << endl;
                }
            }
            break;
        } else if (strcmp(command, "print") == 0) {
            p1 = strtok(NULL, " ");
            print_block(atoi(p1));
        } else if (strcmp(command, "cpout") == 0) {
            p1 = strtok(NULL, " ");
            p2 = strtok(NULL, " ");
            if (!p1 || !p2) {
                cout << "Please provide source and destination file name" << endl;
            } else {
                cpout(p1, p2);
            }
        } else if (strcmp(command, "cd") == 0) {
            p1 = strtok(NULL, " ");
            if (!p1) {
                cout << "Please provide path name" << endl;
            } else {
                cd(p1);
            }
        }
        else if(strcmp(command, "cpin")==0)
		{   
            p1 = strtok(NULL, " ");
			p2 = strtok(NULL, " ");
            if (!p1 || !p2) {
                cout << "Please provide destination and source file name" << endl;
            } else {
                cpin(p1, p2);
            }
        }
		else if(strcmp(command, "rm")==0)
		{
            p1 = strtok(NULL, " ");
			if (!p1) {
                cout << "Please provide path " << endl;
            } else 
			{
                rm(p1);
            }
	}
	else if(strcmp(command, "mkdir") ==0)
	{
		p1= strtok(NULL," ");
		if(!p1)
		{
			cout<<"Please profide the path\n";
		}
		else
		{
			create_directory(p1);
		} 
        } else {
            cout << "Unknown command" << endl;
        }
    }

    return 0;
}
int openfs(char* file_name) {
    fd = open(file_name, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        cout << "Open file failed" << endl;
    } else {
        cout << "Done" << endl;
    }
    return 0;
}

int initfs(int block_num, int inode_num) {
    /* Start initializing super block */
    sb.isize = inode_num;
    sb.fsize = block_num;
    sb.nfree = 1; //Initialization starts from 1 because free[0] should be zero to indicate that there is no free blocks

    for (int i = 0; i < FREE_SIZE; i++) {
        sb.free[i] = 0;
    }

    inode_free = new bool[inode_num + 1];
	for (int i = 1; i <= inode_num; i++) {
		inode_free[i] = true;
	}
    init_root();

    if(fd == -1) {
        cout << "Please open a file first" << endl;
        return -1;
    }

    for (int i = 0; i < BLOCK_SIZE / sizeof(unsigned int); i++) {
        empty_space[i] = 0;
    }

    for (int i = 2 + sb.isize + 1; i < block_num; i++) {    //File system size - boot info - superblock - blocks for inodes - root directory block
        add_block_to_free_list(i);
    }

    lseek(fd, BLOCK_SIZE, SEEK_SET);
    write(fd, &sb, BLOCK_SIZE);

/* cpout test code*/
#if 0
    dir_type an_data;
	
    an_data.inode = 2;
    an_data.filename[0] = '.';
    an_data.filename[1] = '\0';

    lseek(fd, 33 * BLOCK_SIZE, SEEK_SET); //data block# 2
    write(fd, &an_data, sizeof(dir_type));
	
    an_data.inode = 1;
    an_data.filename[0] = '.';
    an_data.filename[1] = '.';
    an_data.filename[2] = '\0';

    write(fd, &an_data, sizeof(dir_type));
	
	an_data.inode = 3;
    an_data.filename[0] = 'o';
    an_data.filename[1] = 'u';
	an_data.filename[2] = 't';
    an_data.filename[3] = '\0';

    write(fd, &an_data, sizeof(dir_type));

	//Create file for cpout test
	inode_type out_inode;
    out_inode.flags = FLAG_INODE_ALLOCATED | FLAG_TYPE_PLAIN_FILE | FLAG_SIZE_SMALL | FLAG_DEFAULT_PERMIT;
	out_inode.addr[0] = 2 + sb.isize + 2;    //Point to data block# 3
	out_inode.size0 = 0;
    out_inode.size1 = 20;
	for (int i = 1; i < INODE_ADDR_SIZE; i++) {
        out_inode.addr[i] = 0;
    }

    lseek(fd, (2 + 2) * BLOCK_SIZE, SEEK_SET);    //i-node# 3
    write(fd, &out_inode, INODE_SIZE);
	
	char buf[20];
	buf[0] = 't';
	buf[1] = 'e';
	buf[2] = 's';
	buf[3] = 't';
	buf[4] = '\0';
	
	for (int i = 5; i < 20; i++) {
		buf[i] = 0;
	}
	
	lseek(fd, 34 * BLOCK_SIZE, SEEK_SET); //data block# 3
	write(fd, &buf, 20);
#endif

    return 0;
}
 void init_root() {
    dir_type root_data;
    int first_data_block = 2 + sb.isize;    //Skip Boot info + Super block +i-lists

    /* Initialize root i-node */
    root_inode.flags = FLAG_INODE_ALLOCATED | FLAG_TYPE_DIRECTORY |
FLAG_SIZE_SMALL | FLAG_DEFAULT_PERMIT; //Should use small file size?
    root_inode.nlinks = 0;
    root_inode.uid = 0;
    root_inode.gid = 0;
    root_inode.size0 = 0;
    root_inode.size1 = 2 * sizeof(dir_type);
    root_inode.addr[0] = 2 + sb.isize;
    for (int i = 1; i < INODE_ADDR_SIZE; i++) {
        root_inode.addr[i] = 0;
    }

    lseek(fd, 2 * BLOCK_SIZE, SEEK_SET);    //Root is at first i-node
    write(fd, &root_inode, INODE_SIZE);
	inode_free[1] = false;

    /* Initialize root data block */
    root_data.inode = 1;
    root_data.filename[0] = '.';
    root_data.filename[1] = '\0';
	for (int i = 2; i < 28; i++) {
		root_data.filename[i] = '\0';
	}

    lseek(fd, first_data_block * BLOCK_SIZE, SEEK_SET);
    write(fd, &root_data, sizeof(dir_type));

    root_data.filename[0] = '.';
    root_data.filename[1] = '.';
    root_data.filename[2] = '\0';
	for (int i = 3; i < 28; i++) {
		root_data.filename[i] = '\0';
	}

    write(fd, &root_data, sizeof(dir_type));
	
	/* cpout test code */
#if 0
	//Create directory for cpout test
    root_data.inode = 2;
	root_data.filename[0] = 'A';
	root_data.filename[1] = 'n';
	root_data.filename[2] = '\0';
	write(fd, &root_data, sizeof(dir_type));

    inode_type an_inode;
	an_inode.flags = FLAG_INODE_ALLOCATED | FLAG_TYPE_DIRECTORY | FLAG_SIZE_SMALL | FLAG_DEFAULT_PERMIT;
	an_inode.addr[0] = 2 + sb.isize + 1;    //Point to data block# 2
	an_inode.size0 = 0;
    an_inode.size1 = 2 * sizeof(dir_type);
	for (int i = 1; i < INODE_ADDR_SIZE; i++) {
        an_inode.addr[i] = 0;
    }

    lseek(fd, (2 + 1) * BLOCK_SIZE, SEEK_SET);    //i-node# 2
    write(fd, &an_inode, INODE_SIZE);
#endif
}

int add_block_to_free_list(int block_number) {
    if (sb.nfree == FREE_SIZE) {
        unsigned int free_block_list[BLOCK_SIZE / sizeof(unsigned int)];
        free_block_list[0] = FREE_SIZE;
        for (int i = 0;i < BLOCK_SIZE / sizeof(unsigned int); i++) {
            if (i < FREE_SIZE) {
                free_block_list[i + 1] = sb.free[i];
            } else {
                free_block_list[i] = 0;
            }
        }
        lseek(fd, block_number * BLOCK_SIZE, SEEK_SET);
        write(fd, &free_block_list, BLOCK_SIZE);
        sb.nfree = 0;
    } else {
        lseek(fd, block_number * BLOCK_SIZE, SEEK_SET);
        write(fd, empty_space, BLOCK_SIZE);
        sb.free[sb.nfree++] = block_number;
    }

    return 0;
}

int q() {
    return close(fd);
}

void print_block (int block_number) {
    unsigned char block_data[BLOCK_SIZE];
    lseek(fd, block_number * BLOCK_SIZE, SEEK_SET);
    read(fd, &block_data,BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (i % 8 == 0) cout << endl;
        printf("%02X ", block_data[i]);
    }
}

void cpout(char* src, char* dst) {
    unsigned int src_inode_num;
    inode_type src_inode;
    enum file_size_type{SMALL, MEDIUM, LONG, SUPER_LONG};
    file_size_type file_size;

    src_inode_num = get_file_inode(src);

    if (src_inode_num == 0) {    //Get source file inode failed
        return;
    }

    lseek(fd, (2 + src_inode_num - 1) * BLOCK_SIZE, SEEK_SET);
    read(fd, &src_inode, sizeof(inode_type));
    file_size = (file_size_type)((src_inode.flags >> 13) & 0x3);

    switch (file_size) {
        case SMALL:
            copy_small_file(src_inode, dst);
            break;
        case MEDIUM:
            copy_medium_file(src_inode, dst);
            break;
        case LONG:
            copy_long_file(src_inode, dst);
            break;
        case SUPER_LONG:
            copy_super_long_file(src_inode, dst);
            break;
    }
}

unsigned int get_file_size(unsigned int size0, unsigned int size1) {
    return ((size0 << 16) & 0xFF) | (size1 & 0xFFFF);
}

int copy_small_file(inode_type src_inode, char *dst) {
    unsigned int file_size;
    unsigned int num_of_full_blocks;
    unsigned int last_block_bytes;
    unsigned char buffer[BLOCK_SIZE];
    int i;

    file_size = get_file_size(src_inode.size0, src_inode.size1);
    num_of_full_blocks = file_size / BLOCK_SIZE;
    last_block_bytes = file_size % BLOCK_SIZE;

    dst_fd = open(dst, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (dst_fd == -1) {
        cout << "copy_small_file: open dst file failed" << endl;
        return -1;
    }

    for (i = 0; i < num_of_full_blocks; i++) {
        lseek(fd, src_inode.addr[i] * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        write(dst_fd, &buffer, BLOCK_SIZE);
    }

    if (last_block_bytes != 0) {
        lseek(fd, src_inode.addr[i] * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, last_block_bytes);
        write(dst_fd, &buffer, last_block_bytes);
    }

    return file_size;
}

int copy_medium_file(inode_type src_inode, char *dst) {
    unsigned int file_size;
    unsigned int num_of_full_blocks;
    unsigned int last_block_bytes;
    unsigned short buffer[BLOCK_SIZE / sizeof(unsigned short)];
    int i;
    unsigned short next_block_num;

    file_size = get_file_size(src_inode.size0, src_inode.size1);
    num_of_full_blocks = file_size / BLOCK_SIZE;
    last_block_bytes = file_size % BLOCK_SIZE;

    dst_fd = open(dst, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (dst_fd == -1) {
        cout << "copy_small_file: open dst file failed" << endl;
        return -1;
    }

    for (i = 0; i < num_of_full_blocks; i++) {
        lseek(fd, src_inode.addr[i / (BLOCK_SIZE / sizeof(unsigned short))] * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        next_block_num = buffer[i % (BLOCK_SIZE / sizeof(unsigned short))];

        lseek(fd, next_block_num * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        write(dst_fd, &buffer, BLOCK_SIZE);
    }

    if (last_block_bytes != 0) {
        lseek(fd, src_inode.addr[i / (BLOCK_SIZE / sizeof(unsigned short))] * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        next_block_num = buffer[i % (BLOCK_SIZE / sizeof(unsigned short))];

        lseek(fd, next_block_num * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, last_block_bytes);
        write(dst_fd, &buffer, last_block_bytes);
    }

    return file_size;
}

int copy_long_file(inode_type src_inode, char *dst) {
    unsigned int file_size;
    unsigned int num_of_full_blocks;
    unsigned int last_block_bytes;
    unsigned short buffer[BLOCK_SIZE / sizeof(unsigned short)];
    int i;
    unsigned short data_block_addr;
    const int count_per_block = BLOCK_SIZE / sizeof(unsigned short);
    unsigned int first_indirect, second_indirect;

    file_size = get_file_size(src_inode.size0, src_inode.size1);
    num_of_full_blocks = file_size / BLOCK_SIZE;
    last_block_bytes = file_size % BLOCK_SIZE;

    dst_fd = open(dst, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (dst_fd == -1) {
        cout << "copy_small_file: open dst file failed" << endl;
        return -1;
    }

    for (i = 0; i <= num_of_full_blocks; i++) {
        //Block num of first level indirect
        first_indirect = i / (count_per_block * count_per_block);
        lseek(fd, src_inode.addr[first_indirect * BLOCK_SIZE], SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        //Block num of second level indirect
        second_indirect = buffer[(i %  (count_per_block * count_per_block)) / count_per_block];
        lseek(fd, second_indirect * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        //Find final data block address
        data_block_addr = buffer[(i %  (count_per_block * count_per_block)) % count_per_block];
        lseek(fd, data_block_addr * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);

        //Copy
        if (i == num_of_full_blocks && last_block_bytes != 0) {
            lseek(fd, data_block_addr * BLOCK_SIZE, SEEK_SET);
            read(fd, &buffer, last_block_bytes);
            write(dst_fd, &buffer, last_block_bytes);
        } else if (i != num_of_full_blocks) {
            lseek(fd, data_block_addr * BLOCK_SIZE, SEEK_SET);
            read(fd, &buffer, BLOCK_SIZE);
            write(dst_fd, &buffer, BLOCK_SIZE);
        }
    }

    return file_size;
}

int copy_super_long_file(inode_type src_inode, char *dst) {
    unsigned int file_size;
    unsigned int num_of_full_blocks;
    unsigned int last_block_bytes;
    unsigned short buffer[BLOCK_SIZE / sizeof(unsigned short)];
    int i;
    unsigned short data_block_addr;
    const int count_per_block = BLOCK_SIZE / sizeof(unsigned short);
    unsigned int first_indirect, second_indirect, third_indirect;

    file_size = get_file_size(src_inode.size0, src_inode.size1);
    num_of_full_blocks = file_size / BLOCK_SIZE;
    last_block_bytes = file_size % BLOCK_SIZE;

    dst_fd = open(dst, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (dst_fd == -1) {
        cout << "copy_small_file: open dst file failed" << endl;
        return -1;
    }

    for (i = 0; i <= num_of_full_blocks; i++) {
        //Block num of first level indirect
        first_indirect = i / (count_per_block * count_per_block * count_per_block);
        lseek(fd, src_inode.addr[first_indirect * BLOCK_SIZE], SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        //Block num of second level indirect
        second_indirect = buffer[(i %  (count_per_block * count_per_block * count_per_block)) / (count_per_block * count_per_block)];
        lseek(fd, second_indirect * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        //Block num of third level indirect
        third_indirect = buffer[((i %  (count_per_block * count_per_block * count_per_block)) % (count_per_block * count_per_block)) / count_per_block];
        lseek(fd, third_indirect * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);
        //Find final data block address
        data_block_addr = buffer[((i %  (count_per_block * count_per_block * count_per_block)) % (count_per_block * count_per_block)) % count_per_block];
        lseek(fd, data_block_addr * BLOCK_SIZE, SEEK_SET);
        read(fd, &buffer, BLOCK_SIZE);

        //Copy
        if (i == num_of_full_blocks && last_block_bytes != 0) {
            lseek(fd, data_block_addr * BLOCK_SIZE, SEEK_SET);
            read(fd, &buffer, last_block_bytes);
            write(dst_fd, &buffer, last_block_bytes);
        } else if (i != num_of_full_blocks) {
            lseek(fd, data_block_addr * BLOCK_SIZE, SEEK_SET);
            read(fd, &buffer, BLOCK_SIZE);
            write(dst_fd, &buffer, BLOCK_SIZE);
        }
    }

    return file_size;
}

unsigned int get_file_inode(char* src) {
    dir_type entry;
    char *next_target = NULL;
    unsigned int next_inode = 1;
    unsigned int next_data_block;
    inode_type inode;
    unsigned short flag = 0;
    int allocated = 0;
    int file_type = -1;
    const int total_entry_num = BLOCK_SIZE / sizeof(dir_type);

    lseek(fd, (2 + cur_inode - 1) * BLOCK_SIZE, SEEK_SET);
    read(fd, &inode, sizeof(inode_type));
    next_data_block = inode.addr[0];
	//cout << "get_file_inode: next_data_block = " << next_data_block << endl;

    next_target = strtok(src, "/");

    do {
        lseek(fd, next_data_block * BLOCK_SIZE, SEEK_SET);
        //if (next_target)
            //cout << "next target = " << next_target << endl;
        for (int i = 0; i <= total_entry_num; i++) {
            if (i == total_entry_num) {
                cout << "cpout: Wrong source file path" << endl;
                return 0;
            }
            read(fd, &entry, sizeof(dir_type));
            //cout << "entry.filename[" << i << "] = " << entry.filename << endl;
            if (strcmp(next_target, (char*) entry.filename) == 0) {
                next_inode = entry.inode;
                break;
            }
        }

        //Read next level directory or file
        //cout << "next_inode = " << next_inode << endl;
        lseek(fd, (2 + next_inode - 1) * BLOCK_SIZE, SEEK_SET);
        read(fd, &inode, sizeof(inode_type));
        allocated = inode.flags >> 15;
        //cout << "allocated = " << allocated << endl;
        file_type = (inode.flags >> 13) & 0x3;
        //cout << "file_type = " << file_type << endl;

        next_target = strtok(NULL, "/");
        next_data_block = inode.addr[0];
    }  while (allocated && file_type == 0x2 && next_target);

    if (!allocated || file_type == 0x2) {
        cout << "cpout: Wrong source file path" << endl;
        return 0;
    }

    return next_inode;
}

unsigned int cd (char* path) {
    unsigned int next_data_block;
    inode_type inode;
    char *next_target = NULL;
    int file_type = -1;
    const int total_entry_num = BLOCK_SIZE / sizeof(dir_type);
    dir_type entry;
    unsigned int next_inode = 1;
    int allocated = 0;

    lseek(fd, (2 + cur_inode - 1) * BLOCK_SIZE, SEEK_SET);    //Go to current i-node
    read(fd, &inode, sizeof(inode_type));
    next_data_block = inode.addr[0];
    next_target = strtok(path, "/");
	cout << "cur_inode = " << cur_inode << endl;

    do {
        lseek(fd, next_data_block * BLOCK_SIZE, SEEK_SET);
        if (next_target)
            cout << "next target = " << next_target << endl;
        for (int i = 0; i <= total_entry_num; i++) {
            if (i == total_entry_num) {
                cout << "cpout: Can not find the directory" << endl;
                return 0;
            }
            read(fd, &entry, sizeof(dir_type));
            cout << "entry.filename[" << i << "] = " << entry.filename << endl;
            if (strcmp(next_target, (char*) entry.filename) == 0) {
                next_inode = entry.inode;
                break;
            }
        }

        //Read next level directory or file
		cur_inode = next_inode;
        cout << "next_inode = " << next_inode << endl;
        lseek(fd, (2 + next_inode - 1) * BLOCK_SIZE, SEEK_SET);
        read(fd, &inode, sizeof(inode_type));
        allocated = inode.flags >> 15;
        //cout << "allocated = " << allocated << endl;
        file_type = (inode.flags >> 13) & 0x3;
        //cout << "file_type = " << file_type << endl;

        next_target = strtok(NULL, "/");
        next_data_block = inode.addr[0];
    }  while (allocated && file_type == 0x2 && next_target);

    if (!allocated || file_type != 0x2) {
        cout << "cpout: Wrong source file path" << endl;
        return 0;
    }

    return next_inode;
}
FILE* fileDescriptor;

void cpin(const char *pathname1 , const char *pathname2)
{
	struct stat stats;
	int blksize, req_blocks;
	int filesize;
	unsigned int new_data_block_num, new_inode_num;
	inode_type new_inode, dir_inode;
	unsigned char buf[BLOCK_SIZE];
	int fd_ext;
	dir_type new_entry;
	int cur_data_block, cur_data_size, i = 0;
	
	stat(pathname1 , &stats);
	filesize = stats.st_size;
	req_blocks = filesize / BLOCK_SIZE;
	cout << "file size = " << filesize << endl;
	
	
	
	/* Create i-node for new file */
	new_inode_num = get_free_inode();
	for (int i = 0; i <= req_blocks; i++) {
		new_inode.addr[i] = request_block_from_free_list();
	}
	for (int i = req_blocks; i < INODE_ADDR_SIZE; i++) {
        new_inode.addr[i] = 0;
    }
	
	new_inode.flags = FLAG_INODE_ALLOCATED | FLAG_TYPE_PLAIN_FILE | FLAG_SIZE_SMALL | FLAG_DEFAULT_PERMIT;
    new_inode.nlinks = 0;
    new_inode.uid = 0;
    new_inode.gid = 0;
    new_inode.size0 = 0;
    new_inode.size1 = filesize;
    new_inode.addr[0] = new_data_block_num;
	
	lseek(fd, (2 + new_inode_num - 1) * BLOCK_SIZE, SEEK_SET);
    write(fd, &new_inode, INODE_SIZE);
	
	/* Add an entry to current working directory i-node */
	new_entry.inode = new_inode_num;
	new_data_block_num = request_block_from_free_list();
	
	while (*pathname2 != '\0' && i < 27) {
		new_entry.filename[i++] = *pathname2++;
	}
	new_entry.filename[i] = '\0';
	
	lseek(fd, (2 + cur_inode - 1) * BLOCK_SIZE, SEEK_SET);
	read(fd, &dir_inode, sizeof(inode_type));
    cur_data_block = dir_inode.addr[0];
	cur_data_size = get_file_size(dir_inode.size0, dir_inode.size1);
    lseek(fd, cur_data_block * BLOCK_SIZE + cur_data_size, SEEK_SET);
	write(fd, &new_entry, sizeof(dir_type));
	
	/* Update current directory file size */
	cur_data_size += sizeof(dir_type);
	dir_inode.size1 = cur_data_size & 0xFFFFFFFF;
	//dir_inode.size0 = cur_data_size & 0xFFFFFFFF00000000;
	lseek(fd, (2 + cur_inode - 1) * BLOCK_SIZE, SEEK_SET);
	write(fd, &dir_inode, sizeof(inode_type));
	
	/* Copy external file to data blocks */
    fd_ext = open(pathname1, O_RDONLY);
	if (fd_ext == -1) {
		cout << "cpin: failed to open source file" << endl;
		return;
	}
	
	for (int i = 0; i <= req_blocks; i++) {
		read(fd_ext, &buf, BLOCK_SIZE);
		lseek(fd, new_inode.addr[i] * BLOCK_SIZE, SEEK_SET);
		write(fd, &buf, BLOCK_SIZE);
	}
}

//Copying from a Small File
/*void smallfile(const char *pathname1 , const char *pathname2 , int blocks_allocated)
	{
	int fdes , fd1 ,i ,j,k,l;
	unsigned short size;
	const char * t;
	const char * d;
	t=pathname2;
	char * e;
	const char * c = (const char *)t;
	d=c;
	e = const_cast<char *>(d);
	unsigned short inode_number;
	inode_number = get_file_inode(e);
	struct stat s1;
	stat(pathname1 , &s1);
	size = s1.st_size;
	unsigned short buff[100];
	inode_type inode;
 	superblock_type sb1;
	sb1.isize = sb.isize;
	sb1.fsize = sb.fsize;
	sb1.nfree = 0;
		for(l=0; l<100; l++)
		{
		sb1.free[sb1.nfree] = l+2+sb1.isize ;
		sb1.nfree++;
		}

	inode.flags = inode_alloc | pfile | 000077; 
	inode.size0 =0;
	inode.size1 = size;
	blocks_allocated = size/512;
	fdes = creat(pathname2, 0775);
	fdes = open(pathname2 , O_RDWR | O_APPEND);
	lseek(fdes , 0 , SEEK_SET);
	write(fdes ,&sb , 512);
	update_rootdir(pathname2 , inode_number);
	unsigned short bl;
	for(i=0; i<blocks_allocated; i++)
		{
		inode.addr[i] = sb.free[i];
		sb.free[i] = 0;
		}
	close(fdes);
	lseek(fdes , 512 , SEEK_SET);
	write(fdes , &inode , 32);
	close(fdes);
	unsigned short buf[256];
	fd1 = open(pathname1 ,O_RDONLY);
	fdes = open(pathname2 , O_RDWR | O_APPEND);
	for(j =0; j<=blocks_allocated; j++)
		{
		lseek(fd1 , 512*j , SEEK_SET);
		read(fd1 ,&buf , 512);
		lseek(fdes , 512*(inode.addr[j]) , SEEK_SET);
		write(fdes , &buf , 512);
		}
	printf("Small file copied\n");
	close(fdes);
	close(fd1);
	fd = open(pathname2 , O_RDONLY); 
	
	}
*/

void update_rootdir(const char *pathname , unsigned short inode_number)
{
	int i;
	dir_type ndir;
	int size;
	unsigned char* t;
	char * e;
	const char * d;
	ndir.inode = inode_number;
	t=ndir.filename;
	const char * c = (const char *)t;
	d=c;
	e = const_cast<char *>(d);
	strncpy(e, pathname , 14);
}

void rm(char* file_name)
{
	unsigned int file_name_inode_num;
    inode_type file_name_inode;
    int size;
    file_name_inode_num = get_file_inode(file_name);
    char *to_delete;
    to_delete = strtok(NULL, " ");
    char* filename = strrchr(to_delete,'/');

    int dblock_addr, i;
    unsigned int buffer[BLOCK_SIZE/4]; // one block buffer
    for (i = 0; i < BLOCK_SIZE/4; i++)
        buffer[i] = 0;
	
	struct stat st;
	stat(file_name, &st);
	size = st.st_size;
	
	int num_of_full_blocks = size / BLOCK_SIZE;
    int last_block_bytes = size % BLOCK_SIZE;
    
    int inode_addr = get_file_inode(file_name);
    inode_type inode = read_inode(inode_addr);
    for ( i=0; i<=num_of_full_blocks; i++){
        dblock_addr = inode.addr[i];
        add_block_to_free_list( dblock_addr,  buffer);
        printf("Please be advise we are releasing the blocks of the file");
    }
    printf("Please be advised next time if you look for the file, it will show error message! ");
    remove(file_name);
}
void add_block_to_free_list(int block_number,  unsigned int *empty_buffer){
    if ( sb.nfree == FREE_SIZE ) {

        int free_list_data[BLOCK_SIZE / 4], i; // 1024 size blocks
        free_list_data[0] = FREE_SIZE;

        for ( i = 0; i < BLOCK_SIZE / 4; i++ ) {
            if ( i < FREE_SIZE ) {
                free_list_data[i + 1] = sb.free[i];
            } else {
                free_list_data[i + 1] = 0; // getting rid of junk data in the remaining unused bytes of header block
            }
        }
        fseek(fileDescriptor, (block_number) * BLOCK_SIZE, SEEK_SET);
        fwrite(free_list_data, BLOCK_SIZE, 1, fileDescriptor); // Writing free list to header block
        sb.nfree = 0;
    } else {
        fseek(fileDescriptor, (block_number) * BLOCK_SIZE, SEEK_SET);
        fwrite(empty_buffer, BLOCK_SIZE, 1, fileDescriptor);  // writing 0 to remaining data blocks to get rid of junk data
    }
    sb.free[sb.nfree] = block_number;  // Assigning blocks to free array
    sb.nfree++;
}
inode_type read_inode(int inode_addr){
    /*reads in an inode struct given the inode_addr*/
    int i_byte_addr = (2*BLOCK_SIZE)+((inode_addr-1)*INODE_SIZE);
    inode_type i_node;
    fseek(fileDescriptor, i_byte_addr, SEEK_SET);
    fread(&i_node, sizeof(inode_type), 1, fileDescriptor);
    return(i_node);
}

int request_block_from_free_list() {
	unsigned int S[BLOCK_SIZE / sizeof(unsigned int)];
	sb.nfree--;
	if (sb.nfree > 0) {
		if (sb.free[sb.nfree] == 0) {
			cout << "out of empty blocks" << endl;
			return -1;
		}
		return sb.free[sb.nfree];
	} else {
		//Read block# free[0] into S
		lseek(fd, sb.free[0] * BLOCK_SIZE, SEEK_SET);    //Root is at first i-node
	    read(fd, &S, BLOCK_SIZE);
		sb.nfree = S[0];
		for (int i = 0; i < FREE_SIZE; i++) {
			sb.free[i] = S[i + 1];
		}
		return sb.free[sb.nfree];
	}
}

int get_free_inode()
{
    for (int i = 1; i <= sb.isize; i++) {
		if (inode_free[i]) {
			inode_free[i] = false;
			cout << "get_free_inode: get free inode num = " << i << endl;
			return i;
		}
	}
	cout << "get_free_inode: no free inode" << endl;
	return -1;
}

void create_directory(char *Dir_name) {
    /* Add a new entry to current working directory */
	inode_type inode, new_inode;
	dir_type new_entry, new_data;
	int new_inode_num, i = 0, new_data_block_num, cur_data_block, cur_data_size;
	
	new_inode_num = get_free_inode();
	new_entry.inode = new_inode_num;
	new_data_block_num = request_block_from_free_list();
	
	cout << "create_directory: cur_inode = " << cur_inode << endl;
	while (*Dir_name && i < 28) {
		new_entry.filename[i++] = *Dir_name++;
	}
	
	lseek(fd, (2 + cur_inode - 1) * BLOCK_SIZE, SEEK_SET);
	read(fd, &inode, sizeof(inode_type));
    cur_data_block = inode.addr[0];
	cur_data_size = get_file_size(inode.size0, inode.size1);
    lseek(fd, cur_data_block * BLOCK_SIZE + cur_data_size, SEEK_SET);
	write(fd, &new_entry, sizeof(dir_type));
	
	//Update current directory file size
	cur_data_size += sizeof(dir_type);
	inode.size1 = cur_data_size & 0xFFFFFFFF;
	//inode.size0 = cur_data_size & 0xFFFFFFFF00000000;
	lseek(fd, (2 + cur_inode - 1) * BLOCK_SIZE, SEEK_SET);
	write(fd, &inode, sizeof(inode_type));
	
	//Write new directory i-node contents
	new_inode.flags = FLAG_INODE_ALLOCATED | FLAG_TYPE_DIRECTORY |
FLAG_SIZE_SMALL | FLAG_DEFAULT_PERMIT; //Should use small file size?
    new_inode.nlinks = 0;
    new_inode.uid = 0;
    new_inode.gid = 0;
    new_inode.size0 = 0;
    new_inode.size1 = 2 * sizeof(dir_type);
    new_inode.addr[0] = new_data_block_num;
    for (int i = 1; i < INODE_ADDR_SIZE; i++) {
        new_inode.addr[i] = 0;
    }

    lseek(fd, (2 + new_inode_num - 1) * BLOCK_SIZE, SEEK_SET);
    write(fd, &new_inode, INODE_SIZE);

    //Write new directory data block contents
	cout << "create_directory: new_data_block_num = " << new_data_block_num << endl;
	
    new_data.inode = new_inode_num;
    new_data.filename[0] = '.';
    new_data.filename[1] = '\0';
	for (int i = 2; i < 28; i++) {
		new_data.filename[i] = '\0';
	}

    lseek(fd, new_data_block_num * BLOCK_SIZE, SEEK_SET);
    write(fd, &new_data, sizeof(dir_type));

    new_data.inode = cur_inode;
    new_data.filename[0] = '.';
    new_data.filename[1] = '.';
    new_data.filename[2] = '\0';
	for (int i = 3; i < 28; i++) {
		new_data.filename[i] = '\0';
	}

    write(fd, &new_data, sizeof(dir_type));
}
