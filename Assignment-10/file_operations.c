#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define INFO_SIZE 1024

int init_File_dd(const char *fname, int bsize, int bno) {
    int fd = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }

    // Write file information (number of blocks and block size) at the beginning
    int info[2] = {bno, bsize};
    write(fd, info, sizeof(int) * 2);

    // Total file size
    long long total_size = INFO_SIZE + (long long)bsize * bno;

    // Seek to end of file to allocate space
    lseek(fd, total_size - 1, SEEK_SET);
    write(fd, "\0", 1); // Write a null byte to allocate space

    close(fd);
    return 0;  // Success
}

int read_block(const char *fname, int bno, char *buffer) {
    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 0; // Failure
    }

    // Read number of blocks and block size
    int info[2];
    read(fd, info, sizeof(int) * 2);
    int n = info[0];
    int s = info[1];

    // Check if bno is within the range of blocks
    if (bno < 0 || bno >= n) {
        close(fd);
        return 0; // Return 0 if bno is out of range
    }

    // Seek to the beginning of the block
    lseek(fd, INFO_SIZE + bno * s, SEEK_SET);
    // Read the block into the buffer
    read(fd, buffer, s);

    close(fd);
    return 1;  // Success
}

int write_block(const char *fname, int bno, char *buffer) {
    int fd = open(fname, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return 0; // Failure
    }

    // Read number of blocks and block size
    int info[2];
    read(fd, info, sizeof(int) * 2);
    int n = info[0];
    int s = info[1];

    // Check if bno is within the range of blocks
    if (bno < 0 || bno >= n) {
        close(fd);
        return 0; // Return 0 if bno is out of range
    }

    // Seek to the beginning of the block
    lseek(fd, INFO_SIZE + bno * s, SEEK_SET);
    // Write the data from buffer to the file
    write(fd, buffer, s);

    close(fd);
    return 1;  // Success
}

int main() {
    const char *fname = "dd2";
    int block_size = 4096;
    int num_blocks = 2048;

    // Initialize the file
    if (init_File_dd(fname, block_size, num_blocks) == -1) {
        printf("Failed to initialize file.\n");
        return 1;
    }

    // Fill buffer with some data
    char buffer[block_size];
    for (int i = 0; i < block_size; i++) {
        buffer[i] = 'a';
    }

    // Write some data to block 0
    if (write_block(fname, 0, buffer) == 1) {
        printf("Data written to block 0 successfully.\n");
    } else {
        printf("Failed to write data to block 0.\n");
    }

    // Read data from block 0
    if (read_block(fname, 0, buffer) == 1) {
        printf("Data read from block 0: ");
        for (int i = 0; i < block_size; i++) {
            printf("%c ", buffer[i]);
        }
        printf("\n");
    } else {
        printf("Failed to read data from block 0.\n");
    }

    return 0;
}