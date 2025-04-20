#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>   
#include <sys/stat.h> 
#include <limits.h>   
#include <unistd.h>    
#include "utilities.c" 

#define MAX_STRING_LENGTH 100

// Add DT_REG definition if not defined
#ifndef DT_REG
#define DT_REG 8
#endif


int *programStartIndex = NULL;
int Program_start_locations[3];


SCHEDULING_ALGORITHM algo;

// initializnig global queues for simple access
MemQueue readyQueueNotPtr;
MemQueue *readyQueue  ;

MemQueue BlockingQueuesNotPtrs[NUM_RESOURCES];
MemQueue *BlockingQueues[NUM_RESOURCES];



MemQueue MLFQ_queues_not_ptrs[4];
MemQueue *MLFQ_queues[4];

// initializing the array to have 3 resources, and so that with the enum i can point to the corresponding resource in a readable format 
bool Resources_availability[NUM_RESOURCES] = {true,true,true};   

MemoryWord memory[60];


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

void PopulateMemory(struct program programList[], int no_of_files)
{
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
        snprintf(memory[programStartIndex[i] + 2].value, MAX_STRING_LENGTH, "%d", programList[i].priority); // how do we get the current priority?
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
//    printf("Variable Storage is full");
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
        return NULL;
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



Resources getResourceType(int program){
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


    if (strcmp(splittedInstruction[0], "semWait") == 0 || strcmp(splittedInstruction[0], "semSignal") == 0 ){
        Resources tmp = (Resources) NULL; 
        if (strcmp(splittedInstruction[1], "userInput") == 0)
            tmp = USER_INPUT;
        else if (strcmp(splittedInstruction[1], "userOutput") == 0)
            tmp = USER_OUTPUT;
        else if (strcmp(splittedInstruction[1], "file") == 0)
            tmp = FILE_ACCESS;
        else {
            perror("invalid resource allocation request");
            exit(EXIT_FAILURE);
        }
        return tmp;
    }
    return (Resources)NULL;
}

bool executeInstruction(int program)
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

        Resources tmp = getResourceType(program);
        Resources_availability[tmp] = false;
    }
    else if (strcmp(splittedInstruction[0], "semSignal") == 0)
    {
        printf("Instruction: %s, Arguments: ", splittedInstruction[0]);
        for (int i = 1; i < instructionCount; i++)
        {
            printf("%s, ", splittedInstruction[i]);
        }
        printf("\n");
        Resources tmp = getResourceType(program);
        Resources_availability[tmp] = true;
        printf("resource we are unblocking: %d\n", tmp);
        
        if (algo != MLFQ){
            while(peek(BlockingQueues[tmp]) != -1){
                int tmp2 =  dequeue(BlockingQueues[tmp]);
                enqueue(readyQueue, tmp2,atoi( memory[programStartIndex[tmp2]+2].value ));
            }
        }else {
            while( peek(BlockingQueues[tmp]) != -1){
                int tmp2 =  dequeue(BlockingQueues[tmp]);
                enqueue(MLFQ_queues[atoi( memory[programStartIndex[tmp2]+2].value )], tmp2, atoi( memory[programStartIndex[tmp2]+2].value ));
            }
        }
    }
    else
    {
        printf("Unknown instruction: %s\n", splittedInstruction[0]);
    }
    instructionIndex++;
    snprintf(memory[programCounterIndex].value, MAX_STRING_LENGTH, "%d", instructionIndex);


    if (atoi(memory[startIndex+4].value2) == atoi(memory[startIndex+3].value)){
        return true; 
    }

    return false;
}



bool can_execute_instruction(int program){
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

    if (strcmp(splittedInstruction[0], "semSignal") == 0 ){
        return true;
    }

    Resources tmp = getResourceType(program);
    if (tmp == NULL){
        return true;
    }else
        return Resources_availability[tmp];            

}


MemQueue*  get_blocking_queue(int program){

    Resources tmp = getResourceType(program);
    return BlockingQueues[tmp];            
    
    //return NULL;          /* <<< and add a safe default here      */
}



void FCFS_algo(struct program programList[] , int num_of_programs){
    int clockcycles = 0;
    int completed = 0;
    while(completed < num_of_programs){
        for (int i = 0; i < num_of_programs; i++){

            // checking if a program should be added into memory
            if ( programList[i].arrivalTime != -1 && clockcycles == programList[i].arrivalTime){
                enqueue(readyQueue,atoi( memory[programStartIndex[i]].value), atoi( memory[programStartIndex[i]+2].value ));
                programList[i].arrivalTime = -1;
            }

        }
        //execute the process that has its turn
        if (peek(readyQueue) != -1){
            if (executeInstruction(peek(readyQueue))){
                dequeue(readyQueue);
                completed++;
            }
        }
        clockcycles++;
    }
}


void RR_algo(struct program programList[] , int num_of_programs, int Quanta ){
    int clockcycles = 0;
    // check arrivals first then move just executed process to back of queue
    int current_quanta = 0;
    int completed = 0;
    int current_process = -1;    

    while(completed < num_of_programs){
        for (int i = 0; i < num_of_programs; i++){
            // checking if a program should be added into memory
            if ( programList[i].arrivalTime != -1 && clockcycles == programList[i].arrivalTime){
                enqueue(readyQueue,atoi( memory[programStartIndex[i]].value), atoi( memory[programStartIndex[i]+2].value ));
                programList[i].arrivalTime = -1;
            }
        }
        printf("Ready Queue: ");
        printQueue(readyQueue, -1);

        // Print out the blocking queues
        for (int r = 0; r < NUM_RESOURCES; r++) {
            printf("Blocking Queue for Resource %d: ", r);
            printQueue(BlockingQueues[r], r);
        }
        //execute the process that has its turn
        if (peek(readyQueue) != -1){

            // finding the next process to execute
            if (current_quanta == 0){
                current_process = peek(readyQueue);
            }
            // continuing the execution of the current process
            // check if we can execute instruction and if so then we execute
            //dumpMemory(Memory_start_location);
            //int pc =  atoi(peek(readyQueue)[3].arg1)+8;
           // printf("trying    => Clock %2d: Running prog %d, PC=%d, instr='%s'\n",clockcycles, atoi(peek(readyQueue)[0].arg1) ,pc,peek(readyQueue)[pc].identifier );
           int pc =  atoi(memory[programStartIndex[current_process]+3].value);
            printf("trying    => Clock %2d: Running prog %d, PC=%d, instr='%s'\n",clockcycles, current_process ,pc,memory[pc].value );
            // Print out the ready queue
            
            if (can_execute_instruction(current_process)){
                int pc =  atoi(memory[programStartIndex[current_process]+3].value);
                printf("executing => Clock %2d: Running prog %d, PC=%d, instr='%s'\n",clockcycles, current_process ,pc,memory[pc].value );
                // executing the instruction, will ready the corresponding blocked processes in case of semSignal  
                if (executeInstruction(current_process)){
                    dequeue(readyQueue);
                    current_quanta = 0;
                    completed++;
                }
                else {
                    current_quanta++;
                    if (current_quanta == Quanta){
                         int tmp=  dequeue(readyQueue);
                        enqueue(readyQueue,tmp, atoi(memory[programStartIndex[tmp]+2].value ));
                        current_quanta = 0;
                    }
                }
                clockcycles++;
            }
            // if we cant execute an instruction it must be due to resource blocking so we must place in the appropriate blocked queue
            else{
                int tmp=  dequeue(readyQueue);
                enqueue(get_blocking_queue(current_process), tmp,atoi(memory[programStartIndex[tmp]+2].value ));
                current_quanta = 0;

            }                
        }else{
            clockcycles++;
        }
    }
}



// void MLFQ_algo(struct program programList[], int number_of_programs) {
//     const int num_levels = 4;
//     // initialize the 4 ready–queues
//     for (int lvl = 0; lvl < num_levels; ++lvl) {
//         initQueue(&MLFQ_queues_not_ptrs[lvl]);
//         MLFQ_queues[lvl] = &MLFQ_queues_not_ptrs[lvl];
//     }
//     // timeslice for each level = 2^(lvl+1): Q0=2, Q1=4, Q2=8, Q3=16
//     int quantum_per_level[num_levels];
//     for (int lvl = 0; lvl < num_levels; ++lvl){
//         quantum_per_level[lvl] = 1 << lvl;
//     }
//     printf("\n");
//     // per‐program MLFQ state
//     for (int p = 0; p < number_of_programs; ++p) {
//         curr_level[p]   = -1;
//         rem_quantum[p]  = 0;
//     }
//     int completed = 0;
//     int clock = 0;
//     struct MemoryWord *running = NULL;
//     int run_pid = -1;
//     while (completed < number_of_programs) {
//         // —— first: unblock any processes just signaled ——
//         for (int r = 0; r < NUM_RESOURCES; ++r) {
//             while (Resources_availability[r] && !isEmpty(BlockingQueues[r])) {
//                 struct MemoryWord *mw = dequeue(BlockingQueues[r]);
//                 int pid = atoi(mw->arg1);
//                 int lvl = curr_level[pid];
//                 enqueue(MLFQ_queues[lvl], mw, 0);
//                 printf("[C=%3d] UNBLOCK → pid=%d back to Q%d\n", clock, pid, lvl);
//             }
//         }
//         // —— arrivals ——
//         for (int p = 0; p < number_of_programs; ++p) {
//             if (programList[p].arrivalTime == clock) {
//                 add_program_to_memory(programList, p, MLFQ_queues[0]);
//                 curr_level[p]  = 0;
//                 rem_quantum[p] = quantum_per_level[0];
//                 //printf("[C=%3d] ARRIVE → pid=%d in Q0\n", clock, p);
//             }
//         }
//         // —— preempt if a higher‐priority queue is non‐empty ——
//         int highest_ready = -1;
//         for (int lvl = 0; lvl < num_levels; ++lvl) {
//             if (!isEmpty(MLFQ_queues[lvl])) { highest_ready = lvl; break; }
//         }
//         if (running && highest_ready != -1 && highest_ready < curr_level[run_pid]) {
//             // preempt current
//            // printf("[C=%3d] PREEMPT → pid=%d lvl=%d rem_q=%d\n",clock, run_pid, curr_level[run_pid], rem_quantum[run_pid]);
//             enqueue(MLFQ_queues[curr_level[run_pid]], running, 0);
//             running = NULL;
//         }
//         // —— dispatch if CPU is free ——
//         if (!running) {
//             int sel_lvl = -1;
//             for (int lvl = 0; lvl < num_levels; ++lvl) {
//                 if (!isEmpty(MLFQ_queues[lvl])) { sel_lvl = lvl; break; }
//             }
//             if (sel_lvl != -1) {
//                 running = dequeue(MLFQ_queues[sel_lvl]);
//                 run_pid = atoi(running->arg1);
//                 // ensure rem_quantum is set (for freshly arrived or demoted)
//                 if (rem_quantum[run_pid] == 0)
//                     rem_quantum[run_pid] = quantum_per_level[sel_lvl];
//                 curr_level[run_pid] = sel_lvl;
//                 //printf("[C=%3d] DISPATCH → pid=%d from Q%d rem_q=%d\n",clock, run_pid, sel_lvl, rem_quantum[run_pid]);
//             }
//         }
//         //print all queues
      
//         // —— execute one time unit ——
//         if (running) {
//             int lvl = curr_level[run_pid];
//             int pc  = atoi(running[3].arg1);
//             char *instr = running[8 + pc].identifier;
//             //printf("[C=%3d] RUN      pid=%d lvl=%d pc=%d instr=\"%s\" rem_q=%d\n", clock, run_pid, lvl, pc, instr, rem_quantum[run_pid]);
//             bool finished = execute_an_instruction(running);
//             if (finished) {
//                 completed++;
//                 //printf("FINISHED → pid=%d (done=%d)\n", run_pid, ++completed);
//                 running = NULL;
//             } else {
//                 rem_quantum[run_pid]--;
//                 if (rem_quantum[run_pid] == 0) {
//                     int old_lvl = lvl;
//                     if (curr_level[run_pid] < num_levels - 1)
//                         curr_level[run_pid]++;
//                     rem_quantum[run_pid] = quantum_per_level[curr_level[run_pid]];
//                     //printf("           TIMESLICE→ pid=%d demote→Q%d rem_q=%d\n",run_pid, curr_level[run_pid], rem_quantum[run_pid]);
//                     enqueue(MLFQ_queues[curr_level[run_pid]], running, 0);
//                     running = NULL;
//                 }
//             }
//         }
//         clock++;
//         // for (int lvl = 0; lvl < num_levels; ++lvl) {
//         //     printf("Q%d: ", lvl);
//         //     printQueue(MLFQ_queues[lvl], lvl);
//         // }
//     }
//     printf("[DONE] All %d progs done at clock %d\n", number_of_programs, clock);
// }


void scheduler(struct program programList[] , int num_of_Programs){
    // initialize ready queue, blocking queue, and running process
    struct MemoryWord *runningProcessLocation = NULL;
    
    initQueue(&readyQueueNotPtr);
    readyQueue = &readyQueueNotPtr;

    for (size_t i = 0; i < NUM_RESOURCES; i++)
    {
        initQueue(&BlockingQueuesNotPtrs[i]);
        BlockingQueues[i] = &BlockingQueuesNotPtrs[i];
    }
    
    //setting the scheduling algorithm
    algo = RR; 
    int quanta = 2; 

    PopulateMemory(programList, num_of_Programs);
    switch (algo)
    {
    case FCFS:
        FCFS_algo(programList, num_of_Programs);
        break;
    case RR:
        RR_algo(programList, num_of_Programs,quanta );
        break;
    case MLFQ:
        //MLFQ_algo(programList, num_of_Programs);
        break;
    }
    
}



int main()
{
    int num_of_programs = count_txt_files("./Programs");
    struct program programList[3] = {
        {"Program_1.txt" , 0, 0 },
        {"Program_2.txt" , 0, 0 },
        {"Program_3.txt" , 0, 0 }
    };  
    scheduler(programList, num_of_programs);
    return 0;
}
