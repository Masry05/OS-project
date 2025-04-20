#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>   // For directory operations
#include <sys/stat.h> // For file status
#include <limits.h>   // For PATH_MAX
#include <unistd.h>   // For additional POSIX functionality

#define MAX_STRING_LENGTH 100

// Add DT_REG definition if not defined
#ifndef DT_REG
#define DT_REG 8
#endif

typedef struct
{
    char name[MAX_STRING_LENGTH];
    char value[MAX_STRING_LENGTH];
    char value2[MAX_STRING_LENGTH];
} MemoryCell;

MemoryCell memory[60];
int *programStartIndex = NULL;

int count_txt_files(const char *directory_path)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(directory_path);
    if (dir == NULL)
    {
        fprintf(stderr, "Failed to open directory: %s\n", directory_path);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        // Use stat to check if it's a regular file instead of d_type
        char full_path[MAX_STRING_LENGTH];
        struct stat st;

        snprintf(full_path, MAX_STRING_LENGTH, "%s/%s", directory_path, entry->d_name);

        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode))
        {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".txt") == 0)
            {
                count++;
            }
        }
    }

    closedir(dir);
    return count;
}

int fillInstructions(const char *filename, int startIndex)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    int ch, i = 0;
    char line[10000];
    int instructionNumber = 0;
    while ((ch = fgetc(file)) != EOF)
    {
        line[i] = ch;
        i++;
        if (ch == '\n')
        {
            line[i] = '\0';
            snprintf(memory[startIndex + instructionNumber].name, MAX_STRING_LENGTH, "Instruction%d", (instructionNumber + 1));
            strcpy(memory[startIndex + instructionNumber].value, line);
            instructionNumber++;
            i = 0;
            line[i] = '\0';
        }
    }
    snprintf(memory[startIndex + instructionNumber].name, MAX_STRING_LENGTH, "Instruction%d", (instructionNumber + 1));
    strcpy(memory[startIndex + instructionNumber].value, line);
    instructionNumber++;
    fclose(file);
    return (startIndex + instructionNumber);
}

void PopulateMemory()
{
    int no_of_files = count_txt_files("./Programs");
    if (no_of_files == -1)
    {
        fprintf(stderr, "Error counting files\n");
        return;
    }
    programStartIndex = (int *)malloc(no_of_files * sizeof(int));
    int currentStartIndex = 0;
    for (int i = 0; i < no_of_files; i++)
    {
        programStartIndex[i] = currentStartIndex;
        strcpy(memory[programStartIndex[i]].name, "Process ID");
        snprintf(memory[programStartIndex[i]].value, MAX_STRING_LENGTH, "%d", i);
        strcpy(memory[programStartIndex[i] + 1].name, "Process Status");
        strcpy(memory[programStartIndex[i] + 1].value, "Ready"); // what are our statuses?
        strcpy(memory[programStartIndex[i] + 2].name, "Current Priority");
        snprintf(memory[programStartIndex[i] + 2].value, MAX_STRING_LENGTH, "%d", 0); // how do we get the current priority?
        strcpy(memory[programStartIndex[i] + 3].name, "Program Counter");
        snprintf(memory[programStartIndex[i] + 3].value, MAX_STRING_LENGTH, "%d", programStartIndex[i] + 8);
        strcpy(memory[programStartIndex[i] + 4].name, "Memory Boundries");
        snprintf(memory[programStartIndex[i] + 4].value, MAX_STRING_LENGTH, "%d", programStartIndex[i]);
        strcpy(memory[programStartIndex[i] + 5].name, "Variable1");
        strcpy(memory[programStartIndex[i] + 5].value, "");
        strcpy(memory[programStartIndex[i] + 5].value2, "");

        // x = 5
        // assign x 5
        // name: esm el cell (Variable1)
        // value: esm el variable (x)
        // value2: el value (5)
        strcpy(memory[programStartIndex[i] + 6].name, "Variable2");
        strcpy(memory[programStartIndex[i] + 6].value, "");
        strcpy(memory[programStartIndex[i] + 6].value2, "");
        strcpy(memory[programStartIndex[i] + 7].name, "Variable3");
        strcpy(memory[programStartIndex[i] + 7].value, "");
        strcpy(memory[programStartIndex[i] + 7].value2, "");

        char filename[MAX_STRING_LENGTH];
        snprintf(filename, MAX_STRING_LENGTH, "./Programs/Program_%d.txt", (i + 1));
        currentStartIndex = fillInstructions(filename, (programStartIndex[i] + 8));
        snprintf(memory[programStartIndex[i] + 4].value2, MAX_STRING_LENGTH, "%d", currentStartIndex - 1);
        if (currentStartIndex == -1)
        {
            fprintf(stderr, "Error filling instructions from file: %s\n", filename);
            return;
        }
    }
}

void PrintMemory()
{
    printf("Memory Content:\n");
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < 60; i++)
    {
        if (strlen(memory[i].name) > 0)
        {
            printf("Cell %d:\n", i);
            printf("  Name: %s\n", memory[i].name);
            printf("  Value: %s\n", memory[i].value);
            if (strlen(memory[i].value2) > 0)
            {
                printf("  Value2: %s\n", memory[i].value2);
            }
            printf("------------------------------------------------------------\n");
        }
    }
}

char **split_string(const char *str, int *count)
{
    if (str == NULL || count == NULL)
        return NULL;

    char *temp = strdup(str);
    if (temp == NULL)
        return NULL;

    int capacity = 10;
    char **result = malloc(capacity * sizeof(char *));
    if (result == NULL)
    {
        free(temp);
        return NULL;
    }

    int index = 0;
    char *token = strtok(temp, " ");
    while (token != NULL)
    {
        if (index >= capacity)
        {
            capacity *= 2;
            char **new_result = realloc(result, capacity * sizeof(char *));
            if (new_result == NULL)
            {
                free(temp);
                for (int i = 0; i < index; i++)
                    free(result[i]);
                free(result);
                return NULL;
            }
            result = new_result;
        }

        result[index] = strdup(token);
        if (result[index] == NULL)
        {
            free(temp);
            for (int i = 0; i < index; i++)
                free(result[i]);
            free(result);
            return NULL;
        }

        index++;
        token = strtok(NULL, " ");
    }

    free(temp);
    *count = index;
    return result;
}

void printVar(int program, const char *varName)
{
    int startIndex = programStartIndex[program];
    for (int i = startIndex + 5; i < startIndex + 8; i++)
    {
        if (strcmp(memory[i].value, varName) == 0)
        {
            printf("Variable: %s, Value: %s\n", memory[i].value, memory[i].value2);
            return;
        }
    }
    printf("Variable %s not found in program %d\n", varName, program);
}

void assign(int program, const char *varName, const char *value)
{
    int startIndex = programStartIndex[program];
    for (int i = startIndex + 5; i < startIndex + 8; i++)
    {
        if (strcmp(memory[i].value, varName) == 0)
        {
            strcpy(memory[i].value2, value);
            break;
        }
        else if (strcmp(memory[i].value, "") == 0)
        {
            strcpy(memory[i].value, varName);
            strcpy(memory[i].value2, value);
            break;
        }
    }
    printf("Variable Storage is full");
}

void writeFile(int program, const char *var1, const char *var2)
{
    char filename[MAX_STRING_LENGTH];
    char data[MAX_STRING_LENGTH];
    int startIndex = programStartIndex[program];
    bool found = false;
    for (int i = startIndex + 5; i < startIndex + 8; i++)
    {
        if (strcmp(memory[i].value, var1) == 0)
        {
            strcpy(filename, memory[i].value2);
            found = true;
            break;
        }
    }
    if (!found)
    {
        printf("Variable %s not found in program %d\n", var1, program);
        return;
    }
    found = false;
    for (int i = startIndex + 5; i < startIndex + 8; i++)
    {
        if (strcmp(memory[i].value, var2) == 0)
        {
            strcpy(data, memory[i].value2);
            found = true;
            break;
        }
    }
    if (!found)
    {
        printf("Variable %s not found in program %d\n", var2, program);
        return;
    }
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Error opening file for writing");
        return;
    }
    fprintf(file, "%s", data);
    fclose(file);
    printf("Data written to file: %s\n", filename);
}

char *readFile(int program, const char *var)
{
    char filename[MAX_STRING_LENGTH];
    int startIndex = programStartIndex[program];
    bool found = false;
    for (int i = startIndex + 5; i < startIndex + 8; i++)
    {
        if (strcmp(memory[i].value, var) == 0)
        {
            strcpy(filename, memory[i].value2);
            found = true;
            break;
        }
    }
    if (!found)
    {
        printf("Variable %s not found in program %d\n", var, program);
        return;
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file for reading");
        return NULL;
    }

    // Allocate memory for the file content
    fseek(file, 0, SEEK_END);    // Move to the end of the file
    long fileSize = ftell(file); // Get the file size
    rewind(file);                // Move back to the beginning of the file

    char *content = (char *)malloc((fileSize + 1) * sizeof(char)); // +1 for null terminator
    if (content == NULL)
    {
        perror("Error allocating memory for file content");
        fclose(file);
        return NULL;
    }

    // Read the file content
    fread(content, sizeof(char), fileSize, file);
    content[fileSize] = '\0'; // Null-terminate the string

    fclose(file);
    return content;
}

void printFromTo(int program, const char *var1, const char *var2)
{
    char num1[MAX_STRING_LENGTH];
    char num2[MAX_STRING_LENGTH];
    int startIndex = programStartIndex[program];
    bool found = false;
    for (int i = startIndex + 5; i < startIndex + 8; i++)
    {
        if (strcmp(memory[i].value, var1) == 0)
        {
            strcpy(num1, memory[i].value2);
            found = true;
            break;
        }
    }
    if (!found)
    {
        printf("Variable %s not found in program %d\n", var1, program);
        return;
    }
    found = false;
    for (int i = startIndex + 5; i < startIndex + 8; i++)
    {
        if (strcmp(memory[i].value, var2) == 0)
        {
            strcpy(num2, memory[i].value2);
            found = true;
            break;
        }
    }
    if (!found)
    {
        printf("Variable %s not found in program %d\n", var2, program);
        return;
    }
    int startNum = atoi(num1);
    int endNum = atoi(num2);
    if (startNum > endNum)
    {
        int temp = startNum;
        startNum = endNum;
        endNum = temp;
    }
    for (int i = startNum; i <= endNum; i++)
    {
        printf("%d ", i);
    }
    printf("\n");
}

void executeInstruction(int program)
{
    int startIndex = programStartIndex[program];
    int programCounterIndex = startIndex + 3;
    int instructionIndex = atoi(memory[programCounterIndex].value);
    char instruction[MAX_STRING_LENGTH];
    strncpy(instruction, memory[instructionIndex].value, MAX_STRING_LENGTH - 1);
    instruction[MAX_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
    if (instruction[strlen(instruction) - 1] == '\n')
        instruction[strlen(instruction) - 1] = '\0'; // Remove newline character
    int instructionCount = 0;
    char **splittedInstruction = split_string(instruction, &instructionCount);
    if (strcmp(splittedInstruction[0], "print") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
        printVar(program, splittedInstruction[1]);
    }
    else if (strcmp(splittedInstruction[0], "assign") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
        if (instructionCount == 3)
        {
            assign(program, splittedInstruction[1], splittedInstruction[2]);
        }
        else if (instructionCount == 4)
        {
            char *fileContent = readFile(program, splittedInstruction[3]);
            if (fileContent != NULL)
            {
                assign(program, splittedInstruction[1], fileContent);
                free(fileContent); // Free the allocated memory after use
            }
        }
    }
    else if (strcmp(splittedInstruction[0], "writeFile") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
        writeFile(program, splittedInstruction[1], splittedInstruction[2]);
    }
    else if (strcmp(splittedInstruction[0], "readFile") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
        readFile(program, splittedInstruction[1]);
    }
    else if (strcmp(splittedInstruction[0], "printFromTo") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
        printFromTo(program, splittedInstruction[1], splittedInstruction[2]);
    }
    else if (strcmp(splittedInstruction[0], "semWait") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
    }
    else if (strcmp(splittedInstruction[0], "semSignal") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
    }
    else
    {
        printf("Unknown instruction: %s\n", splittedInstruction[0]);
    }
    instructionIndex++;
    snprintf(memory[programCounterIndex].value, MAX_STRING_LENGTH, "%d", instructionIndex);
}

int main()
{
    PopulateMemory();
    PrintMemory();
    return 0;
}
