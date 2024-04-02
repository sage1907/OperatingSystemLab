#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_FILES 100
#define BLOCK_SIZE 512
#define MAX_FILE_NAME 256

// Structure to represent a file in myfs
typedef struct {
   char name[MAX_FILE_NAME];
   int size;
   int blocks[MAX_FILES];
   int numBlocks;
   int current_position; // New member to track current position
} MyFile;

// Structure to represent the myfs file system
typedef struct {
   char file_name[MAX_FILE_NAME];
   int numFiles;
   MyFile files[MAX_FILES];
   int ddid; // Disk descriptor ID
} MyFS;

// Global variable to store the mounted myfs
MyFS *mountedFS = NULL;

// Function prototypes
int init_File_dd(const char *fname, int bsize, int bno);
int read_block_dd(int ddid, int bno, char *buffer);
int write_block_dd(int ddid, int bno, char *buffer);
int readblock(const char *fname, int ch_no, int bno, char *buffer);
int writeblock(const char *fname, int ch_no, int bno, char *buffer);
void mymkfs(const char *filename);
void mymount(const char *filename);
void myumount();
void mylist();
int myopen(const char *filename);
int mycreat(const char *filename);
void myunlink(const char *filename);
int myread(int file_desc, char *buffer, int size);
int mywrite(int file_desc, char *buffer, int size);
int mylseek(int file_desc, int offset, int whence);
void myclose(int file_desc);
void mycopy(const char *src_filename, const char *dest_filename);
void mycopyFromOS(const char *os_filename, const char *myfs_filename);
void mycopyToOS(const char *myfs_filename, const char *os_filename);
void free_block(int block_number);


int main() {
   int choice;
   char filename[MAX_FILE_NAME];
   char src_filename[MAX_FILE_NAME];
   char dest_filename[MAX_FILE_NAME];

   while (1) {
       printf("\nMyFS Menu:\n");
       printf("1. Create myfs\n");
       printf("2. Mount myfs\n");
       printf("3. Unmount myfs\n");
       printf("4. List files in myfs\n");
       printf("5. Create file in myfs\n");
       printf("6. Delete file from myfs\n");
       printf("7. Copy file within myfs\n");
       printf("8. Copy file from OS to myfs\n");
       printf("9. Copy file from myfs to OS\n");
       printf("10. Exit\n");
       printf("Enter your choice: ");
       scanf("%d", &choice);

       switch (choice) {
           case 1:
               printf("Enter filename for myfs: ");
               scanf("%s", filename);
               mymkfs(filename);
               break;
           case 2:
               printf("Enter filename for myfs: ");
               scanf("%s", filename);
               mymount(filename);
               break;
           case 3:
               myumount();
               break;
           case 4:
               mylist();
               break;
           case 5:
               printf("Enter filename: ");
               scanf("%s", filename);
               mycreat(filename);
               break;
           case 6:
               printf("Enter filename: ");
               scanf("%s", filename);
               myunlink(filename);
               break;
           case 7:
               printf("Enter source filename: ");
               scanf("%s", src_filename);
               printf("Enter destination filename: ");
               scanf("%s", dest_filename);
               mycopy(src_filename, dest_filename);
               break;
           case 8:
               printf("Enter OS filename: ");
               scanf("%s", src_filename);
               printf("Enter myfs filename: ");
               scanf("%s", dest_filename);
               mycopyFromOS(src_filename, dest_filename);
               break;
           case 9:
               printf("Enter myfs filename: ");
               scanf("%s", src_filename);
               printf("Enter OS filename: ");
               scanf("%s", dest_filename);
               mycopyToOS(src_filename, dest_filename);
               break;
           case 10:
               exit(0);
           default:
               printf("Invalid choice\n");
       }
   }

   return 0;
}


// Function to initialize a new file system
int init_File_dd(const char *fname, int bsize, int bno) {
    int fd = open(fname, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        perror("Error opening file");
        return -1;
    }

    // Create an empty file system with zeroed blocks
    char buffer[BLOCK_SIZE] = {0};
    for (int i = 0; i < bno; i++) {
        if (write(fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
            perror("Error writing to file");
            close(fd);
            return -1;
        }
    }

    return fd;
}

// Function to read a block from disk
int read_block_dd(int ddid, int bno, char *buffer) {
    off_t offset = bno * BLOCK_SIZE;
    if (lseek(ddid, offset, SEEK_SET) == -1) {
        perror("Error seeking file");
        return -1;
    }

    if (read(ddid, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Error reading from file");
        return -1;
    }

    return 0;
}

// Function to write a block to disk
int write_block_dd(int ddid, int bno, char *buffer) {
    off_t offset = bno * BLOCK_SIZE;
    if (lseek(ddid, offset, SEEK_SET) == -1) {
        perror("Error seeking file");
        return -1;
    }

    if (write(ddid, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Error writing to file");
        return -1;
    }

    return 0;
}

// Function to read a block from the disk
int readblock(const char *fname, int ch_no, int bno, char *buffer) {
    int fd = open(fname, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return -1;
    }

    if (lseek(fd, ch_no * BLOCK_SIZE + bno * BLOCK_SIZE, SEEK_SET) == -1) {
        perror("Error seeking file");
        close(fd);
        return -1;
    }

    if (read(fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Error reading from file");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

// Function to write a block to the disk
int writeblock(const char *fname, int ch_no, int bno, char *buffer) {
    int fd = open(fname, O_WRONLY);
    if (fd < 0) {
        perror("Error opening file");
        return -1;
    }

    if (lseek(fd, ch_no * BLOCK_SIZE + bno * BLOCK_SIZE, SEEK_SET) == -1) {
        perror("Error seeking file");
        close(fd);
        return -1;
    }

    if (write(fd, buffer, BLOCK_SIZE) != BLOCK_SIZE) {
        perror("Error writing to file");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}



// Function to create a new myfs file system
void mymkfs(const char *filename) {
   MyFS *fs = (MyFS *)malloc(sizeof(MyFS));
   strcpy(fs->file_name, filename);
   fs->numFiles = 0;
   fs->ddid = init_File_dd(filename, BLOCK_SIZE, 0);

   if (fs->ddid < 0) {
       printf("Error creating myfs\n");
       free(fs);
       return;
   }

   printf("myfs created successfully\n");
}


// Function to mount an existing myfs file system
void mymount(const char *filename) {
   if (mountedFS != NULL) {
       printf("myfs already mounted\n");
       return;
   }

   MyFS *fs = (MyFS *)malloc(sizeof(MyFS));
   strcpy(fs->file_name, filename);
   fs->ddid = init_File_dd(filename, BLOCK_SIZE, 0);

   if (fs->ddid < 0) {
       printf("Error mounting myfs\n");
       free(fs);
       return;
   }

   // Read the file system metadata from the first block
   char buffer[BLOCK_SIZE];
   read_block_dd(fs->ddid, 0, buffer);
   memcpy(&(fs->numFiles), buffer, sizeof(int));
   memcpy(fs->files, buffer + sizeof(int), sizeof(MyFile) * MAX_FILES);

   mountedFS = fs;
   printf("myfs mounted successfully\n");
}


// Function to unmount the currently mounted myfs
void myumount() {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return;
   }

   // Write the file system metadata to the first block
   char buffer[BLOCK_SIZE];
   memcpy(buffer, &(mountedFS->numFiles), sizeof(int));
   memcpy(buffer + sizeof(int), mountedFS->files, sizeof(MyFile) * MAX_FILES);
   write_block_dd(mountedFS->ddid, 0, buffer);

   free(mountedFS);
   mountedFS = NULL;
   printf("myfs unmounted successfully\n");
}


// Function to list all files in the currently mounted myfs
void mylist() {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return;
   }

   printf("Files in myfs:\n");
   for (int i = 0; i < mountedFS->numFiles; i++) {
       printf("%s\n", mountedFS->files[i].name);
   }
}


// Function to open a file in the currently mounted myfs
int myopen(const char *filename) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return -1;
   }

   for (int i = 0; i < mountedFS->numFiles; i++) {
       if (strcmp(mountedFS->files[i].name, filename) == 0) {
           return i; // Return file descriptor (index in files array)
       }
   }

   printf("File not found: %s\n", filename);
   return -1;
}


// Function to create a new file in the currently mounted myfs
int mycreat(const char *filename) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return -1;
   }

   if (mountedFS->numFiles == MAX_FILES) {
       printf("Maximum number of files reached\n");
       return -1;
   }

   for (int i = 0; i < mountedFS->numFiles; i++) {
       if (strcmp(mountedFS->files[i].name, filename) == 0) {
           printf("File already exists: %s\n", filename);
           return -1;
       }
   }

   MyFile newFile;
   strcpy(newFile.name, filename);
   newFile.size = 0;
   newFile.numBlocks = 0;

   mountedFS->files[mountedFS->numFiles] = newFile;
   mountedFS->numFiles++;

   return mountedFS->numFiles - 1; // Return file descriptor (index in files array)
}


// Function to delete a file from the currently mounted myfs
void myunlink(const char *filename) {
    if (mountedFS == NULL) {
        printf("No myfs mounted\n");
        return;
    }

    for (int i = 0; i < mountedFS->numFiles; i++) {
        if (strcmp(mountedFS->files[i].name, filename) == 0) {
            // Free the blocks used by the file
            for (int j = 0; j < mountedFS->files[i].numBlocks; j++) {
                free_block(mountedFS->files[i].blocks[j]);
            }

            // Shift the remaining files in the array
            for (int j = i + 1; j < mountedFS->numFiles; j++) {
                mountedFS->files[j - 1] = mountedFS->files[j];
            }

            mountedFS->numFiles--;
            printf("File deleted: %s\n", filename);
            return;
        }
    }

    printf("File not found: %s\n", filename);
}



// Function to read from a file in the currently mounted myfs
int myread(int file_desc, char *buffer, int size) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return -1;
   }

   if (file_desc < 0 || file_desc >= mountedFS->numFiles) {
       printf("Invalid file descriptor\n");
       return -1;
   }

   MyFile *file = &(mountedFS->files[file_desc]);
   int bytesRead = 0;
   int totalBytesRead = 0;
   int remainingBytes = size;

   // Calculate the number of bytes left to read based on file size and current position
   int bytesLeft = file->size - file->current_position;

   // Adjust size if it exceeds the remaining bytes in the file
   if (size > bytesLeft) {
       size = bytesLeft;
   }

   // Calculate the block index and position within the block based on the current position
   int blockIndex = file->current_position / BLOCK_SIZE;
   int blockOffset = file->current_position % BLOCK_SIZE;

   // Loop until all requested bytes are read or until the end of the file
   while (totalBytesRead < size && blockIndex < file->numBlocks) {
       char blockBuffer[BLOCK_SIZE];
       // Read the block containing the current position
       readblock(mountedFS->file_name, file->blocks[blockIndex], 0, blockBuffer);

       // Calculate the number of bytes to read from the current block
       int bytesToRead = BLOCK_SIZE - blockOffset;
       if (bytesToRead > remainingBytes) {
           bytesToRead = remainingBytes;
       }

       // Copy data from the block buffer to the output buffer
       memcpy(buffer + totalBytesRead, blockBuffer + blockOffset, bytesToRead);

       // Update counters and positions
       totalBytesRead += bytesToRead;
       remainingBytes -= bytesToRead;
       bytesRead += bytesToRead;
       file->current_position += bytesToRead;
       blockIndex++;
       blockOffset = 0; // Reset block offset for subsequent blocks
   }

   return bytesRead;
}


// Function to write to a file in the currently mounted myfs
int mywrite(int file_desc, char *buffer, int size) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return -1;
   }

   if (file_desc < 0 || file_desc >= mountedFS->numFiles) {
       printf("Invalid file descriptor\n");
       return -1;
   }

   MyFile *file = &(mountedFS->files[file_desc]);
   int bytesWritten = 0;
   int totalBytesWritten = 0;

   // Calculate the block index and position within the block based on the current position
   int blockIndex = file->current_position / BLOCK_SIZE;
   int blockOffset = file->current_position % BLOCK_SIZE;

   // Loop until all requested bytes are written or until the end of the file
   while (totalBytesWritten < size && blockIndex < MAX_FILES) {
       char blockBuffer[BLOCK_SIZE];
       // Read the block containing the current position
       readblock(mountedFS->file_name, file->blocks[blockIndex], 0, blockBuffer);

       // Calculate the number of bytes to write to the current block
       int bytesToWrite = BLOCK_SIZE - blockOffset;
       if (bytesToWrite > size - totalBytesWritten) {
           bytesToWrite = size - totalBytesWritten;
       }

       // Copy data from the input buffer to the block buffer
       memcpy(blockBuffer + blockOffset, buffer + totalBytesWritten, bytesToWrite);

       // Write the updated block buffer back to disk
       writeblock(mountedFS->file_name, file->blocks[blockIndex], 0, blockBuffer);

       // Update counters and positions
       totalBytesWritten += bytesToWrite;
       bytesWritten += bytesToWrite;
       file->current_position += bytesToWrite;
       blockIndex++;
       blockOffset = 0; // Reset block offset for subsequent blocks
   }

   // Update file size if necessary
   if (file->current_position > file->size) {
       file->size = file->current_position;
   }

   return bytesWritten;
}


// Function to change the current position in a file in the currently mounted myfs
int mylseek(int file_desc, int offset, int whence) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return -1;
   }

   if (file_desc < 0 || file_desc >= mountedFS->numFiles) {
       printf("Invalid file descriptor\n");
       return -1;
   }

   MyFile *file = &(mountedFS->files[file_desc]);
   int newPosition;

   switch (whence) {
       case SEEK_SET:
           newPosition = offset;
           break;
       case SEEK_CUR:
           newPosition = file->current_position + offset;
           break;
       case SEEK_END:
           newPosition = file->size + offset;
           break;
       default:
           printf("Invalid whence parameter\n");
           return -1;
   }

   // Ensure newPosition is within the bounds of the file
   if (newPosition < 0) {
       newPosition = 0;
   } else if (newPosition > file->size) {
       newPosition = file->size;
   }

   file->current_position = newPosition;

   return newPosition;
}


// Function to close a file in the currently mounted myfs
void myclose(int file_desc) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return;
   }

   if (file_desc < 0 || file_desc >= mountedFS->numFiles) {
       printf("Invalid file descriptor\n");
       return;
   }

   // No action needed for closing a file in this implementation
}


// Function to copy a file within the currently mounted myfs
void mycopy(const char *src_filename, const char *dest_filename) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return;
   }

   int src_fd = myopen(src_filename);
   if (src_fd < 0) {
       printf("Source file not found: %s\n", src_filename);
       return;
   }

   int dest_fd = mycreat(dest_filename);
   if (dest_fd < 0) {
       printf("Error creating destination file: %s\n", dest_filename);
       return;
   }

   MyFile *src_file = &(mountedFS->files[src_fd]);
   MyFile *dest_file = &(mountedFS->files[dest_fd]);

   char buffer[BLOCK_SIZE];
   int bytesRead, bytesWritten;

   // TODO: Implement copying from one file to another within myfs

   myclose(src_fd);
   myclose(dest_fd);
}


// Function to copy a file from the OS to the currently mounted myfs
void mycopyFromOS(const char *os_filename, const char *myfs_filename) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return;
   }

   int os_fd = open(os_filename, O_RDONLY);
   if (os_fd < 0) {
       printf("Error opening OS file: %s\n", os_filename);
       return;
   }

   int myfs_fd = mycreat(myfs_filename);
   if (myfs_fd < 0) {
       printf("Error creating myfs file: %s\n", myfs_filename);
       close(os_fd);
       return;
   }

   MyFile *file = &(mountedFS->files[myfs_fd]);

   char buffer[BLOCK_SIZE];
   int bytesRead, bytesWritten;

   while ((bytesRead = read(os_fd, buffer, BLOCK_SIZE)) > 0) {
       bytesWritten = mywrite(myfs_fd, buffer, bytesRead);
       if (bytesWritten < 0) {
           printf("Error writing to myfs file\n");
           break;
       }
   }

   close(os_fd);
   myclose(myfs_fd);
}


// Function to copy a file from the currently mounted myfs to the OS
void mycopyToOS(const char *myfs_filename, const char *os_filename) {
   if (mountedFS == NULL) {
       printf("No myfs mounted\n");
       return;
   }

   int myfs_fd = myopen(myfs_filename);
   if (myfs_fd < 0) {
       printf("myfs file not found: %s\n", myfs_filename);
       return;
   }

   int os_fd = open(os_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
   if (os_fd < 0) {
       printf("Error creating OS file: %s\n", os_filename);
       return;
   }

   MyFile *file = &(mountedFS->files[myfs_fd]);

   char buffer[BLOCK_SIZE];
   int bytesRead, bytesWritten;

   while ((bytesRead = myread(myfs_fd, buffer, BLOCK_SIZE)) > 0) {
       bytesWritten = write(os_fd, buffer, bytesRead);
       if (bytesWritten < 0) {
           printf("Error writing to OS file\n");
           break;
       }
   }

   close(os_fd);
   myclose(myfs_fd);
}

void free_block(int block_number) {
    printf("Block %d freed successfully\n", block_number);
}
