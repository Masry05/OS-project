#ifndef shared_functions_h
#define shared_functions_h

#include <gtk/gtk.h>

// Declare the callback function (just a prototype)
void auto_execution(GtkWidget *widget, gpointer user_data);
void step_execution(GtkWidget *widget, gpointer user_data);
void reset_execution(GtkWidget *widget, gpointer user_data);
void reset_all();
void intialize_dashboard();
void update_pcb();
void backButtonPressed();

// Declare setup function

#endif