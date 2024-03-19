#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define MAX_DESC_SIZE 500
#define MAX_NAME_SIZE 50

typedef struct
{
    int roll;
    char fname[MAX_NAME_SIZE];
    char mname[MAX_NAME_SIZE];
    char sname[MAX_NAME_SIZE];
    char desc[MAX_DESC_SIZE];
} studentRecord;

char *dataFile;
char *indexFile;


int readIntFromBuffer(char **buffer)
{
    int value;
    memcpy(&value, *buffer, sizeof(int));
    *buffer += sizeof(int);
    return value;
}


void readStringFromBuffer(char **bufferName, char *target, int size)
{
    memcpy(target, *bufferName, size);
    *bufferName += size;
    target[size] = '\0';
}


studentRecord deserialiseBuffer(char *bufferName)
{
    studentRecord st;

    st.roll = readIntFromBuffer(&bufferName);
    readStringFromBuffer(&bufferName, st.fname, 50);
    readStringFromBuffer(&bufferName, st.mname, 50);
    readStringFromBuffer(&bufferName, st.sname, 50);
    int descLength = readIntFromBuffer(&bufferName);
    strncpy(st.desc, bufferName, descLength);
    st.desc[descLength] = '\0';
    return st;
}


int readEntryCount(int fileDes)
{
    lseek(fileDes, 0, SEEK_SET);
    int count;
    read(fileDes, &count, sizeof(int));
    return count;
}


void updateEntryCount(int fileDes)
{
    lseek(fileDes, 0, SEEK_SET);
    int n;
    read(fileDes, &n, sizeof(int));
    n++;
    lseek(fileDes, 0, SEEK_SET);
    write(fileDes, &n, sizeof(int));
}


void insertRecord(studentRecord *st)
{
    int dataFD = open(dataFile, O_RDWR | O_CREAT, 0666);
    int indexFD = open(indexFile, O_RDWR | O_CREAT, 0666);

    // Ensure initial count is written if files are new
    off_t dataEnd = lseek(dataFD, 0, SEEK_END);
    if (dataEnd == 0)
    {
        int initialCount = 0;
        write(dataFD, &initialCount, sizeof(int));
    }

    off_t indexEnd = lseek(indexFD, 0, SEEK_END);
    if (indexEnd == 0)
    {
        int initialCount = 0;
        write(indexFD, &initialCount, sizeof(int));
    }

    // Serialize the details
    char *buffer = (char *)malloc(sizeof(char) * 1024);
    int offset = 0;

    memcpy(buffer + offset, &st->roll, sizeof(int));
    offset += sizeof(int);
    memcpy(buffer + offset, st->fname, MAX_NAME_SIZE);
    offset += MAX_NAME_SIZE;
    memcpy(buffer + offset, st->mname, MAX_NAME_SIZE);
    offset += MAX_NAME_SIZE;
    memcpy(buffer + offset, st->sname, MAX_NAME_SIZE);
    offset += MAX_NAME_SIZE;
    int descLength = strlen(st->desc);
    memcpy(buffer + offset, &descLength, sizeof(int));
    offset += sizeof(int);
    memcpy(buffer + offset, st->desc, descLength);
    offset += descLength;

    
    off_t recordPosition = lseek(dataFD, 0, SEEK_END);

    
    write(dataFD, buffer, offset);

    
    write(indexFD, &recordPosition, sizeof(off_t));

    
    updateEntryCount(dataFD);
    updateEntryCount(indexFD);

    free(buffer);
    close(dataFD);
    close(indexFD);
}


studentRecord searchRecord(int roll, int *flag)
{
    int indexFD = open(indexFile, O_RDONLY);
    if (indexFD == -1)
    {
        perror("open failed\n");
        exit(EXIT_FAILURE);
    }
    int dataFD = open(dataFile, O_RDONLY);
    if (dataFD == -1)
    {
        perror("open failed\n");
        exit(EXIT_FAILURE);
    }

    int n = readEntryCount(indexFD); 
    *flag = 0;                       

    studentRecord st;
    memset(&st, 0, sizeof(st)); 
    for (int i = 0; i < n; i++)
    {
        off_t offset;
        
        if (read(indexFD, &offset, sizeof(off_t)) != sizeof(off_t))
        {
            perror("Failed to read offset from index file");
            exit(EXIT_FAILURE);
        }

        
        if (lseek(dataFD, offset, SEEK_SET) == (off_t)-1)
        {
            perror("Failed to seek in data file");
            exit(EXIT_FAILURE);
        }

        
        char *buffer = (char *)malloc(sizeof(char) * 1024);
        int bytesRead = read(dataFD, buffer, sizeof(char) * 1024);
        if (bytesRead == -1)
        {
            perror("Failed to read data");
            exit(EXIT_FAILURE);
        }


        char *tempBuffer = buffer;
        st = deserialiseBuffer(tempBuffer);

        if (st.roll == roll)
        {
            *flag = 1;
            break;
        }
        free(buffer);
    }

    close(indexFD);
    close(dataFD);
    return st;
}



void compactData(int indexFD, int dataFD, int deleteIndex, int totalEntries)
{
    studentRecord st;
    off_t newIndex[totalEntries - 1], newOffset = sizeof(int);
    int tempDataFD = open("tempData.data", O_RDWR | O_CREAT | O_TRUNC, 0666);
    int tempIndexFD = open("tempIndex.index", O_RDWR | O_CREAT | O_TRUNC, 0666);

    if (tempDataFD == -1 || tempIndexFD == -1)
    {
        perror("Creating temporary files failed");
        exit(EXIT_FAILURE);
    }

    
    int newCount = totalEntries - 1;
    write(tempDataFD, &newCount, sizeof(int));
    write(tempIndexFD, &newCount, sizeof(int));

    lseek(dataFD, sizeof(int), SEEK_SET);
    lseek(indexFD, sizeof(int), SEEK_SET);

    for (int i = 0; i < totalEntries; i++)
    {
        off_t offset;
        read(indexFD, &offset, sizeof(off_t));

       
        if (i == deleteIndex)
            continue; 

        lseek(dataFD, offset, SEEK_SET);
        read(dataFD, &st, sizeof(studentRecord));

        write(tempDataFD, &st, sizeof(studentRecord));
        newIndex[i < deleteIndex ? i : i - 1] = newOffset;
        newOffset += sizeof(studentRecord);
    }

    
    for (int i = 0; i < newCount; i++)
    {
        write(tempIndexFD, &newIndex[i], sizeof(off_t));
    }

    rename("tempData.data", dataFile);
    rename("tempIndex.index", indexFile);

    close(tempDataFD);
    close(tempIndexFD);
}


int deleteRecord(int roll)
{
    int flag = 0, indexFD, dataFD, n;
    off_t offset;
    studentRecord st;

    indexFD = open(indexFile, O_RDWR);
    dataFD = open(dataFile, O_RDWR);

    if (indexFD == -1 || dataFD == -1)
    {
        perror("Opening file failed");
        exit(EXIT_FAILURE);
    }

    n = readEntryCount(indexFD); 

    for (int i = 0; i < n; i++)
    {
        read(indexFD, &offset, sizeof(off_t));

        lseek(dataFD, offset, SEEK_SET);          
        read(dataFD, &st, sizeof(studentRecord));

        if (st.roll == roll)
        {
            flag = 1;
            compactData(indexFD, dataFD, i, n);
            break;
        }
    }

    close(indexFD);
    close(dataFD);
    return flag;
}


int modifyRecord(int roll, studentRecord *newRecord)
{
    if (deleteRecord(roll) == 0)
    {
        return 0;
    }
    insertRecord(newRecord);
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <path of data file> <path of index file>\n", argv[0]);
        return 0;
    }
    dataFile = argv[1];
    indexFile = argv[2];

    while (1)
    {
            printf("Enter your choice for the given actions :-\n 1. Insert a Student Record\n 2. Search for a Student Record\n 3. Modify a Student Record\n 4. Delete a Student Record\n 5. Exit from process\n");
        int choice;
        scanf("%d", &choice);
        if (choice == 1)
        {
            
            studentRecord *st = (studentRecord *)malloc(sizeof(studentRecord));
            printf("Please enter the student's roll no: \n");
            scanf("%d", &st->roll);
            printf("Please enter the student's first name: \n");
            scanf("%s", st->fname);
            st->fname[sizeof(st->fname) - 1] = '\0';
            printf("Please enter the student's middle name: \n");
            scanf("%s", st->mname);
            st->mname[sizeof(st->mname) - 1] = '\0';
            printf("Please enter the student's surname: \n");
            scanf("%s", st->sname);
            st->sname[sizeof(st->sname) - 1] = '\0';
            printf("Give a brief description of the student: \n");
            getc(stdin);
            scanf("%[^\n]s", st->desc); // read until \n is encountered
            st->desc[sizeof(st->desc) - 1] = '\0';
            printf("Details of the student:- \n");
            printf("Roll No:- %d\n", st->roll);
            printf("Name:-  %s %s %s\n", st->fname, st->mname, st->sname);
            printf("Description:- \n %s\n", st->desc);
            printf("Adding these details ...\n");
            insertRecord(st);
            printf("Record has been saved.\n");
            free(st);
        }
        else if (choice == 2)
        {            
            printf("Enter the roll-number\n");
            int roll, flag;
            scanf("%d", &roll);
            studentRecord st;
            printf("Searching started ...\n");
            st = searchRecord(roll, &flag);
            if (!flag)
            {
                printf("No such record exists\n");
                continue;
            }
            printf("Details of the student:- \n");
            printf("Roll No:- %d\n", st.roll);
            printf("Name:- %s %s %s\n", st.fname, st.mname, st.sname);
            printf("Description:- \n %s\n", st.desc);
        }
        else if (choice == 3)
        {
            printf("Enter the roll-number\n");
            int roll;
            scanf("%d", &roll);
            studentRecord *st = (studentRecord *)malloc(sizeof(studentRecord));
            printf("Provide the new Details\n");
            printf("Enter the roll number of student\n");
            scanf("%d", &st->roll);
            printf("Enter the first name of the student\n");
            scanf("%s", st->fname);
            st->fname[sizeof(st->fname) - 1] = '\0';
            printf("Enter the middle name of the student\n");
            scanf("%s", st->mname);
            st->mname[sizeof(st->mname) - 1] = '\0';
            printf("Enter the surname of the student\n");
            scanf("%s", st->sname);
            st->sname[sizeof(st->sname) - 1] = '\0';
            printf("Give a brief description of the student\n");
            getc(stdin);
            scanf("%[^\n]s", st->desc);
            st->desc[sizeof(st->desc) - 1] = '\0';
            printf("Modifying the details ...\n");
            if (modifyRecord(roll, st) == 0)
            {
                printf("Record cannot be modified\n");
                continue;
            }
            printf("Record has been modified.\n");
        }
        else if (choice == 4)
        {
            int roll;
            printf("Enter the roll-number\n");
            scanf("%d", &roll);
            printf("Deleting the record ...\n");
            if (deleteRecord(roll) == 0)
            {
                printf("Record cannot be deleted\n");
                continue;
            }
            printf("Record has been deleted\n");
        }
        else if (choice == 5)
        {
            // Exit
            printf("Exiting Program... Thank you\n");
            break;
        }
        else
        {
            printf("Wrong choice Try again.\n");
        }
    }
    return 0;
}