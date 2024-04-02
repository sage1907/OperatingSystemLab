#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

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
   fs->ddid = init_File_dd(filename, BLOCK_SIZE, MAX_FILES);
   if (fs->ddid != -1) {
       mountedFS = fs;
       printf("myfs created successfully.\n");
   }
}

// Function to mount an existing myfs file system
void mymount(const char *filename) {
   MyFS *fs = (MyFS *)malloc(sizeof(MyFS));
   strcpy(fs->file_name, filename);
   fs->numFiles = 0;
   fs->ddid = open(filename, O_RDWR);
   if (fs->ddid != -1) {
       mountedFS = fs;
       printf("myfs mounted successfully.\n");
   }
}

// Function to unmount the current myfs file system
void myumount() {
   if (mountedFS) {
       close(mountedFS->ddid);
       free(mountedFS);
       mountedFS = NULL;
       printf("myfs unmounted successfully.\n");
   } else {
       printf("No myfs mounted.\n");
   }
}

// Function to list all files in the myfs file system
void mylist() {
   if (mountedFS) {
       printf("Files in myfs:\n");
       for (int i = 0; i < mountedFS->numFiles; i++) {
           printf("%s\n", mountedFS->files[i].name);
       }
   } else {
       printf("No myfs mounted.\n");
   }
}

// Function to open a file in myfs
int myopen(const char *filename) {
   if (!mountedFS) {
       printf("No myfs mounted.\n");
       return -1;
   }
   
   for (int i = 0; i < mountedFS->numFiles; i++) {
       if (strcmp(mountedFS->files[i].name, filename) == 0) {
           mountedFS->files[i].current_position = 0;
           return i;
       }
   }

   printf("File not found in myfs.\n");
   return -1;
}

// Function to create a file in myfs
int mycreat(const char *filename) {
   if (!mountedFS) {
       printf("No myfs mounted.\n");
       return -1;
   }
   
   if (mountedFS->numFiles >= MAX_FILES) {
       printf("Cannot create more files. Maximum limit reached.\n");
       return -1;
   }

   for (int i = 0; i < mountedFS->numFiles; i++) {
       if (strcmp(mountedFS->files[i].name, filename) == 0) {
           printf("File already exists in myfs.\n");
           return -1;
       }
   }

   MyFile newFile;
   strcpy(newFile.name, filename);
   newFile.size = 0;
   newFile.numBlocks = 0;
   mountedFS->files[mountedFS->numFiles++] = newFile;
   return mountedFS->numFiles - 1;
}

// Function to delete a file from myfs
void myunlink(const char *filename) {
   if (!mountedFS) {
       printf("No myfs mounted.\n");
       return;
   }
   
   for (int i = 0; i < mountedFS->numFiles; i++) {
       if (strcmp(mountedFS->files[i].name, filename) == 0) {
           for (int j = i; j < mountedFS->numFiles - 1; j++) {
               mountedFS->files[j] = mountedFS->files[j + 1];
           }
           mountedFS->numFiles--;
           printf("File %s deleted from myfs.\n", filename);
           return;
       }
   }

   printf("File not found in myfs.\n");
}

// Function to read from an opened file in myfs
int myread(int file_desc, char *buffer, int size) {
   if (!mountedFS) {
       printf("No myfs mounted.\n");
       return -1;
   }
   
   if (file_desc < 0 || file_desc >= mountedFS->numFiles) {
       printf("Invalid file descriptor.\n");
       return -1;
   }
   
   MyFile *file = &(mountedFS->files[file_desc]);
   if (file->current_position + size > file->size) {
       printf("Attempted to read beyond file size.\n");
       return -1;
   }

   char tempBuffer[BLOCK_SIZE];
   int bytesRead = 0;
   int remainingSize = size;
   while (remainingSize > 0) {
       int blockNo = file->current_position / BLOCK_SIZE;
       int offset = file->current_position % BLOCK_SIZE;
       int bytesToRead = remainingSize > BLOCK_SIZE - offset ? BLOCK_SIZE - offset : remainingSize;
       if (readblock(mountedFS->file_name, file->blocks[blockNo], offset, tempBuffer) == -1) {
           printf("Error reading from file.\n");
           return -1;
       }
       memcpy(buffer + bytesRead, tempBuffer, bytesToRead);
       bytesRead += bytesToRead;
       remainingSize -= bytesToRead;
       file->current_position += bytesToRead;
   }

   return bytesRead;
}

// Function to write to an opened file in myfs
int mywrite(int file_desc, char *buffer, int size) {
   if (!mountedFS) {
       printf("No myfs mounted.\n");
       return -1;
   }
   
   if (file_desc < 0 || file_desc >= mountedFS->numFiles) {
       printf("Invalid file descriptor.\n");
       return -1;
   }
   
   MyFile *file = &(mountedFS->files[file_desc]);
   char tempBuffer[BLOCK_SIZE];
   int bytesWritten = 0;
   int remainingSize = size;
   while (remainingSize > 0) {
       int blockNo = file->current_position / BLOCK_SIZE;
       int offset = file->current_position % BLOCK_SIZE;
       int bytesToWrite = remainingSize > BLOCK_SIZE - offset ? BLOCK_SIZE - offset : remainingSize;
       if (offset == 0 && file->numBlocks < MAX_FILES) {
           int newBlockNo = file->numBlocks++;
           file->blocks[blockNo] = newBlockNo;
       }
       if (offset == 0 && file->current_position == file->size) {
           file->size += bytesToWrite;
       }
       memcpy(tempBuffer, buffer + bytesWritten, bytesToWrite);
       if (writeblock(mountedFS->file_name, file->blocks[blockNo], offset, tempBuffer) == -1) {
           printf("Error writing to file.\n");
           return -1;
       }
       bytesWritten += bytesToWrite;
       remainingSize -= bytesToWrite;
       file->current_position += bytesToWrite;
   }

   return bytesWritten;
}

// Function to change the current position in an opened file in myfs
int mylseek(int file_desc, int offset, int whence) {
   if (!mountedFS) {
       printf("No myfs mounted.\n");
       return -1;
   }
   
   if (file_desc < 0 || file_desc >= mountedFS->numFiles) {
       printf("Invalid file descriptor.\n");
       return -1;
   }
   
   MyFile *file = &(mountedFS->files[file_desc]);
   int newPosition = -1;
   if (whence == SEEK_SET) {
       newPosition = offset;
   } else if (whence == SEEK_CUR) {
       newPosition = file->current_position + offset;
   } else if (whence == SEEK_END) {
       newPosition = file->size + offset;
   } else {
       printf("Invalid whence parameter.\n");
       return -1;
   }

   if (newPosition < 0 || newPosition > file->size) {
       printf("Invalid seek position.\n");
       return -1;
   }

   file->current_position = newPosition;
   return newPosition;
}

// Function to close an opened file in myfs
void myclose(int file_desc) {
   if (!mountedFS) {
       printf("No myfs mounted.\n");
       return;
   }
   
   if (file_desc < 0 || file_desc >= mountedFS->numFiles) {
       printf("Invalid file descriptor.\n");
       return;
   }

   printf("File %s closed.\n", mountedFS->files[file_desc].name);
}

// Function to copy a file within myfs
void mycopy(const char *src_filename, const char *dest_filename) {
   int src_fd = myopen(src_filename);
   if (src_fd == -1) {
       printf("Unable to open source file.\n");
       return;
   }

   int dest_fd = mycreat(dest_filename);
   if (dest_fd == -1) {
       printf("Unable to create destination file.\n");
       myclose(src_fd);
       return;
   }

   char buffer[BLOCK_SIZE];
   int bytesRead;
   while ((bytesRead = myread(src_fd, buffer, BLOCK_SIZE)) > 0) {
       if (mywrite(dest_fd, buffer, bytesRead) == -1) {
           printf("Error writing to destination file.\n");
           myclose(src_fd);
           myclose(dest_fd);
           return;
       }
   }

   myclose(src_fd);
   myclose(dest_fd);
   printf("File copied successfully within myfs.\n");
}

// Function to copy a file from OS to myfs
void mycopyFromOS(const char *os_filename, const char *myfs_filename) {
   int os_fd = open(os_filename, O_RDONLY);
   if (os_fd == -1) {
       printf("Unable to open source OS file.\n");
       return;
   }

   int dest_fd = mycreat(myfs_filename);
   if (dest_fd == -1) {
       printf("Unable to create destination file in myfs.\n");
       close(os_fd);
       return;
   }

   char buffer[BLOCK_SIZE];
   int bytesRead;
   while ((bytesRead = read(os_fd, buffer, BLOCK_SIZE)) > 0) {
       if (mywrite(dest_fd, buffer, bytesRead) == -1) {
           printf("Error writing to destination file in myfs.\n");
           close(os_fd);
           myclose(dest_fd);
           return;
       }
   }

   close(os_fd);
   myclose(dest_fd);
   printf("File copied successfully from OS to myfs.\n");
}

// Function to copy a file from myfs to OS
void mycopyToOS(const char *myfs_filename, const char *os_filename) {
   int src_fd = myopen(myfs_filename);
   if (src_fd == -1) {
       printf("Unable to open source file in myfs.\n");
       return;
   }

   int os_fd = open(os_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
   if (os_fd == -1) {
       printf("Unable to create destination OS file.\n");
       myclose(src_fd);
       return;
   }

   char buffer[BLOCK_SIZE];
   int bytesRead;
   while ((bytesRead = myread(src_fd, buffer, BLOCK_SIZE)) > 0) {
       if (write(os_fd, buffer, bytesRead) == -1) {
           printf("Error writing to destination OS file.\n");
           myclose(src_fd);
           close(os_fd);
           return;
       }
   }

   myclose(src_fd);
   close(os_fd);
   printf("File copied successfully from myfs to OS.\n");
}

int main() {
   char choice;
   char filename[100];
   char dest_filename[100];
   char os_filename[100];

   while (1) {
       printf("\nMENU:\n");
       printf("1. Create myfs\n");
       printf("2. Mount myfs\n");
       printf("3. Unmount myfs\n");
       printf("4. List files in myfs\n");
       printf("5. Open file\n");
       printf("6. Create file\n");
       printf("7. Delete file\n");
       printf("8. Read from file\n");
       printf("9. Write to file\n");
       printf("10. Seek in file\n");
       printf("11. Close file\n");
       printf("12. Copy file within myfs\n");
       printf("13. Copy file from OS to myfs\n");
       printf("14. Copy file from myfs to OS\n");
       printf("15. Exit\n");
       printf("Enter your choice: ");
       scanf(" %c", &choice);

       switch (choice) {
           case '1':
               printf("Enter filename for myfs: ");
               scanf("%s", filename);
               mymkfs(filename);
               break;
           case '2':
               printf("Enter filename to mount: ");
               scanf("%s", filename);
               mymount(filename);
               break;
           case '3':
               myumount();
               break;
           case '4':
               mylist();
               break;
           case '5':
               printf("Enter filename to open: ");
               scanf("%s", filename);
               myopen(filename);
               break;
           case '6':
               printf("Enter filename to create: ");
               scanf("%s", filename);
               mycreat(filename);
               break;
           case '7':
               printf("Enter filename to delete: ");
               scanf("%s", filename);
               myunlink(filename);
               break;
           case '8':
               // Implement read operation
	       if (mountedFS) {
               int file_desc;
               printf("Enter file descriptor: ");
               scanf("%d", &file_desc);
               char buffer[100];
               printf("Enter number of bytes to read: ");
               int size;
               scanf("%d", &size);
               int bytes_read = myread(file_desc, buffer, size);
               if (bytes_read > 0) {
                   printf("Read %d bytes: %s\n", bytes_read, buffer);
               } else {
                   printf("Failed to read from file.\n");
               }
    	       } else {
                   printf("No myfs mounted.\n");
    	       }
               break;
           case '9':
               // Implement write operation
	       if (mountedFS) {
               int file_desc;
               printf("Enter file descriptor: ");
               scanf("%d", &file_desc);
               char buffer[100];
               printf("Enter data to write: ");
               scanf("%s", buffer);
               int bytes_written = mywrite(file_desc, buffer, strlen(buffer));
               if (bytes_written > 0) {
                   printf("Wrote %d bytes.\n", bytes_written);
               } else {
                   printf("Failed to write to file.\n");
               }
               } else {
               printf("No myfs mounted.\n");
               }
               break;
           case '10':
               // Implement seek operation
		if (mountedFS) {
        	int file_desc;
        	printf("Enter file descriptor: ");
        	scanf("%d", &file_desc);
        	int offset, whence;
        	printf("Enter offset: ");
        	scanf("%d", &offset);
        	printf("Enter whence (SEEK_SET=0, SEEK_CUR=1, SEEK_END=2): ");
        	scanf("%d", &whence);
        	int new_pos = mylseek(file_desc, offset, whence);
        	if (new_pos >= 0) {
            	printf("New position: %d\n", new_pos);
        	} else {
            	printf("Seek operation failed.\n");
        	}
    		} else {
        	printf("No myfs mounted.\n");
    		}
                break;
           case '11':
               // Implement close operation
		 if (mountedFS) {
        	int file_desc;
        	printf("Enter file descriptor: ");
        	scanf("%d", &file_desc);
        	myclose(file_desc);
        	printf("File closed.\n");
    		} else {
        	printf("No myfs mounted.\n");
    		}
                break;
           case '12':
               printf("Enter source filename: ");
               scanf("%s", filename);
               printf("Enter destination filename: ");
               scanf("%s", dest_filename);
               mycopy(filename, dest_filename);
               break;
           case '13':
               printf("Enter source OS filename: ");
               scanf("%s", os_filename);
               printf("Enter destination myfs filename: ");
               scanf("%s", filename);
               mycopyFromOS(os_filename, filename);
               break;
           case '14':
               printf("Enter source myfs filename: ");
               scanf("%s", filename);
               printf("Enter destination OS filename: ");
               scanf("%s", os_filename);
               mycopyToOS(filename, os_filename);
               break;
           case '15':
               printf("Exiting program.\n");
               exit(0);
           default:
               printf("Invalid choice. Please try again.\n");
       }
   }

   return 0;
}
