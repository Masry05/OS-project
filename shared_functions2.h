#ifndef shared_functions2_h
#define shared_functions2_h

// Declare the callback function (just a prototype)
void update();
char *assignInput();
void printFromToGUI(char *result);
void updateMutex(int program, int resource, bool status);
void updateExecutionLog(const char *instruction, const char *pid, const char *reaction);
void addEventMessage(char *message);
// Declare setup function

#endif