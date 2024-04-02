#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define INFO_SIZE 1024

typedef struct Superblock {
    int n;        // Total number of blocks in the file
    int s;        // Size of each block
    int f;        // Number of free blocks
    unsigned char *fb;  // Bit pattern showing which blocks are free
    int *chains;  // Array to store the block number of the 1st block of each chain
} Superblock;

int init_File_dd(const char *fname, int bsize, int bno) {
    int fd = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }

    // Allocate memory for superblock
    Superblock super;
    super.n = bno;
    super.s = bsize;
    super.f = bno;
    super.fb = (unsigned char *)calloc((bno + 7) / 8, sizeof(unsigned char));
    memset(super.fb, 0, (bno + 7) / 8); // Initially all blocks are free

    // Allocate memory for chains array and initialize with -1
    super.chains = (int *)malloc(bno * sizeof(int));
    memset(super.chains, -1, bno * sizeof(int));

    // Write superblock to file
    write(fd, &super, sizeof(Superblock));

    close(fd);

    return 0;  // Success
}

int readblock(const char *fname, int ch_no, int bno, char *buffer) {
    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 0; // Failure
    }

    // Read superblock
    Superblock super;
    read(fd, &super, sizeof(Superblock));

    // Check if chain exists
    if (super.chains[ch_no] == -1) {
        close(fd);
        return 0; // Chain does not exist
    }

    // Seek to the beginning of the requested block in the chain
    lseek(fd, INFO_SIZE + super.chains[ch_no] * super.s + bno * super.s, SEEK_SET);

    // Read the block into the buffer
    read(fd, buffer, super.s);

    close(fd);
    return 1;  // Success
}

int writeblock(const char *fname, int ch_no, int bno, char *buffer) {
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return 0; // Failure
    }

    // Read superblock
    Superblock super;
    read(fd, &super, sizeof(Superblock));

    // Check if chain exists, if not, create the chain
    if (super.chains[ch_no] == -1) {
        // Find a free block to start the chain
        int free_block = -1;
        for (int i = 0; i < super.n; i++) {
            if (!(super.fb[i / 8] & (1 << (i % 8)))) {
                free_block = i;
                break;
            }
        }
        if (free_block == -1) {
            close(fd);
            return 0; // No free blocks available
        }
        // Mark the block as used
        super.fb[free_block / 8] |= (1 << (free_block % 8));
        super.f--; // Decrease free blocks count
        super.chains[ch_no] = free_block; // Set the first block of the chain

        // Update superblock in the file
        lseek(fd, 0, SEEK_SET);
        write(fd, &super, sizeof(Superblock));
    }

    // Seek to the beginning of the requested block in the chain
    lseek(fd, INFO_SIZE + super.chains[ch_no] * super.s + bno * super.s, SEEK_SET);

    // Write the data from buffer to the file
    write(fd, buffer, super.s);

    close(fd);
    return 1;  // Success
}

int main() {
    const char *fname = "dd2";
    int block_size = 4096;
    int num_blocks = 2048;
    int choice, chain_no, block_no;
    char *buffer = (char *)malloc(block_size * sizeof(char));

    // Initialize the file
    if (init_File_dd(fname, block_size, num_blocks) == -1) {
        printf("Failed to initialize file.\n");
        return 1;
    }

    do {
        // Menu
        printf("\nMenu:\n");
        printf("1. Read block\n");
        printf("2. Write block\n");
        printf("3. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1:
                printf("Enter chain number: ");
                scanf("%d", &chain_no);
                getchar();
                printf("Enter block number: ");
                scanf("%d", &block_no);
                getchar();
                if (readblock(fname, chain_no, block_no, buffer) == 1) {
                    printf("Data read from chain %d, block %d: %s\n", chain_no, block_no, buffer);
                } else {
                    printf("Failed to read data from chain %d, block %d.\n", chain_no, block_no);
                }
                break;
            case 2:
                printf("Enter chain number: ");
                scanf("%d", &chain_no);
                getchar();
                printf("Enter block number: ");
                scanf("%d", &block_no);
                getchar();
                printf("Enter data to write: ");
                fgets(buffer, block_size, stdin);
                buffer[strcspn(buffer, "\n")] = '\0';
                if (writeblock(fname, chain_no, block_no, buffer) == 1) {
                    printf("Data written to chain %d, block %d successfully.\n", chain_no, block_no);
                } else {
                    printf("Failed to write data to chain %d, block %d.\n", chain_no, block_no);
                }
                break;
            case 3:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice!\n");
        }
    } while (choice != 3);

    free(buffer);
    return 0;
}