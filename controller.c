#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "gui.c"
#include "backend.c"

extern SCHEDULING_ALGORITHM algo;
extern int RR_quantum;

void auto_execution(GtkWidget *widget)
{
    g_print("Auto execution button clicked.\n");
    scheduler();
}

void set_scheduler(GtkWidget *widget)
{
    if (GTK_IS_LABEL(widget))
    {
        const gchar *text = gtk_label_get_text(GTK_LABEL(widget));
        g_print("Label text: %s\n", text);
    }
    else if (GTK_IS_COMBO_BOX_TEXT(widget))
    {
        gchar *text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
        if (text)
        {
            if (strcmp(text, "First Come First Serve") == 0)
            {
                algo = FCFS;
                g_print("Selected: First Come First Serve\n");
            }
            else if (strcmp(text, "Round Robin") == 0)
            {
                algo = RR;

                g_print("Selected: Round Robin\n");
            }
            else if (strcmp(text, "Multilevel Feedback Queue") == 0)
            {
                algo = MLFQ;
                g_print("Selected: Multilevel Feedback Queue\n");
            }
            else
            {
                g_print("Unknown algorithm selected.\n");
            }
            g_print("ComboBoxText selected: %s\n", text);
            g_free(text);
        }
        else
        {
            g_print("ComboBoxText has no selection.\n");
        }
    }
}

static void activate(GtkApplication *app, gpointer user_data)
{
    g_print("— activate() has been called —\n");
    application = app;
    // Create the initial window
    GtkWidget *initial_window = create_initial_window(app);
    gtk_application_add_window(app, GTK_WINDOW(initial_window));
    g_timeout_add(0, (GSourceFunc)gtk_widget_hide, quantum_button_box);
    gtk_widget_show_all(initial_window);
    g_signal_connect(dropdown, "changed", G_CALLBACK(set_scheduler), NULL);
    g_signal_connect(G_OBJECT(auto_button), "clicked", G_CALLBACK(auto_execution), data);
}

int main(int argc, char *argv[])
{
    int status;

    // Initialize process_list_store
    GType process_types[] = {G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING};
    process_list_store = gtk_list_store_newv(5, process_types);

    // Initialize mutex_status_store
    GType mutex_types[] = {G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING};
    mutex_status_store = gtk_list_store_newv(3, mutex_types);

    // Initialize memory_viewer_store
    GType memory_types[] = {G_TYPE_STRING, G_TYPE_STRING};
    memory_viewer_store = gtk_list_store_newv(2, memory_types);

    // Initialize log_store
    GType log_types[] = {G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING};
    log_store = gtk_list_store_newv(3, log_types);

    application = gtk_application_new("simulation.scheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(application, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);

    // Clean up
    if (process_list_store)
    {
        g_object_unref(process_list_store);
        process_list_store = NULL;
    }
    if (mutex_status_store)
    {
        g_object_unref(mutex_status_store);
        mutex_status_store = NULL;
    }
    if (memory_viewer_store)
    {
        g_object_unref(memory_viewer_store);
        memory_viewer_store = NULL;
    }
    if (log_store)
    {
        g_object_unref(log_store);
        log_store = NULL;
    }
    free_process_list();

    return status;
}