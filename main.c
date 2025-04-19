#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_STRING_LENGTH 100

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
    WIN32_FIND_DATAA find_data;
    char search_path[MAX_PATH];
    int count = 0;

    snprintf(search_path, sizeof(search_path), "%s\\*.txt", directory_path);

    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open directory: %s\n", directory_path);
        return -1;
    }

    do
    {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            count++;
        }
    } while (FindNextFileA(hFind, &find_data) != 0);

    FindClose(hFind);
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
        // x = 5
        // assign x 5
        // name: esm el cell (Variable1)
        // value: esm el variable (x)
        // value2: el value (5)
        strcpy(memory[programStartIndex[i] + 6].name, "Variable2");
        strcpy(memory[programStartIndex[i] + 7].name, "Variable3");
        char filename[MAX_PATH];
        snprintf(filename, MAX_PATH, "./Programs/Program_%d.txt", (i + 1));
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
    }
    else if (strcmp(splittedInstruction[0], "assign") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
    }
    else if (strcmp(splittedInstruction[0], "writeFile") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
    }
    else if (strcmp(splittedInstruction[0], "readFile") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
    }
    else if (strcmp(splittedInstruction[0], "printFromTo") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
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
}
int main()
{
    PopulateMemory();
    PrintMemory();
    executeInstruction(1);
    return 0;
}
