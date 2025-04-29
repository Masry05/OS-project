#ifndef shared_functions_h
#define shared_functions_h

#include <gtk/gtk.h>

// Declare the callback function (just a prototype)
void auto_execution(GtkWidget *widget, gpointer user_data);
void step_execution(GtkWidget *widget, gpointer user_data);
void reset_execution(GtkWidget *widget, gpointer user_data);

void update_pcb();

// Declare setup function

#endif