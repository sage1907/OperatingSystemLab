#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_DATA_SIZE 100

// Enumeration to represent different data types
typedef enum
{
    INT_TYPE,
    FLOAT_TYPE,
    DOUBLE_TYPE,
    CHAR_TYPE,
    STRING_TYPE
} DataType;

typedef struct
{
    DataType type;
    union
    {
        int intData;
        float floatData;
        double doubleData;
        char charData;
        char stringData[MAX_DATA_SIZE];
    } data;
} Data;

void enqueue(Data *data, int write_fd)
{
    if (write(write_fd, data, sizeof(Data)) == -1)
    {
        perror("write() failed");
        exit(2);
    }
}

void dequeue(Data *data, int read_fd)
{
    if (read(read_fd, data, sizeof(Data)) == -1)
    {
        perror("read() failed");
        exit(4);
    }
}

// Function to get user input based on data type
void getUserInput(Data *data)
{
    printf("Choose data type:\n");
    printf("1. Integer\n");
    printf("2. Float\n");
    printf("3. Double\n");
    printf("4. Char\n");
    printf("5. String\n");
    printf("Enter your choice (or enter 0 to stop enqueuing): ");

    int choice;
    scanf("%d", &choice);

    if (choice == 0)
    {
        data->type = -1; // Marking the end of data
    }
    else
    {
        switch (choice)
        {
        case 1:
            data->type = INT_TYPE;
            printf("Enter an integer: ");
            scanf("%d", &data->data.intData);
            break;
        case 2:
            data->type = FLOAT_TYPE;
            printf("Enter a float: ");
            scanf("%f", &data->data.floatData);
            break;
        case 3:
            data->type = DOUBLE_TYPE;
            printf("Enter a double: ");
            scanf("%lf", &data->data.doubleData);
            break;
        case 4:
            data->type = CHAR_TYPE;
            printf("Enter a character: ");
            scanf(" %c", &data->data.charData);
            break;
        case 5:
            data->type = STRING_TYPE;
            printf("Enter a string: ");
            scanf("%s", data->data.stringData);
            break;
        default:
            printf("Invalid choice\n");
            exit(1);
        }
    }
}

int main()
{
    int pipefds[2];
    int flag;

    // Creating a pipe
    flag = pipe(pipefds);
    if (flag == -1)
    {
        perror("pipe() failed");
        exit(1);
    }

    Data buf;

    while (1)
    {
        getUserInput(&buf);
        enqueue(&buf, pipefds[1]);

        if (buf.type == -1)
        {
            break;
        }
    }

    Data dequeuedData;

    while (1)
    {
        dequeue(&dequeuedData, pipefds[0]);

        if (dequeuedData.type == -1)
        {
            printf("\n\nQueue empty\n\n");
            break;
        }

        switch (dequeuedData.type)
        {
        case INT_TYPE:
            printf("%d\n", dequeuedData.data.intData);
            break;
        case FLOAT_TYPE:
            printf("%f\n", dequeuedData.data.floatData);
            break;
        case DOUBLE_TYPE:
            printf("%lf\n", dequeuedData.data.doubleData);
            break;
        case CHAR_TYPE:
            printf("%c\n", dequeuedData.data.charData);
            break;
        case STRING_TYPE:
            printf("%s\n", dequeuedData.data.stringData);
            break;
        default:
            printf("Unknown data type\n");
        }
    }

    return 0;
}