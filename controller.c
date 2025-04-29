#include <stdio.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gui.c"
#include "backend.c"

extern int RR_quantum;

SCHEDULING_ALGORITHM algo;
bool stepper;
bool alreadyRunning;

void update_pcb()
{
    // printf("Updating PCB\n");

    // Clear the process_list_store before updating it
    if (process_list_store && GTK_IS_LIST_STORE(process_list_store))
    {
        gtk_list_store_clear(process_list_store);
    }

    for (int i = 0; i < total_processes; i++)
    {
        char range[256];
        strcpy(range, memory[programStartIndex[i] + 4].value);
        strcat(range, "-");
        strcat(range, memory[programStartIndex[i] + 4].value2);

        gtk_list_store_insert_with_values(process_list_store, NULL, -1,
                                          0, memory[programStartIndex[i]].value,
                                          1, memory[programStartIndex[i] + 1].value,
                                          2, memory[programStartIndex[i] + 2].value,
                                          3, range,
                                          4, memory[programStartIndex[i] + 3].value,
                                          -1);
    }
}

void update_queue(MemQueue readyQueue, DynamicQueueWidget ready_queue)
{
    // free_dynamic_queue(&ready_queue);
    for (int i = 0; i < total_processes + 1; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            const char *value = "";
            gtk_label_set_text(GTK_LABEL(ready_queue.value_labels[i][j]), value);
        }
    }

    if ((&readyQueue) != NULL && (&readyQueue)->size > 0)
    {
        for (size_t i = 0; i < (&readyQueue)->size; i++)
        {
            char ready_value0[100]; // Temporary buffers
            char ready_value1[100];
            char ready_value2[100];

            strcpy(ready_value0, memory[programStartIndex[(&readyQueue)->items[i].ptr]].value);

            int pc = atoi(memory[programStartIndex[(&readyQueue)->items[i].ptr] + 3].value);
            strcpy(ready_value1, memory[pc].value);

            int arrival_tmp = programs_arrival_list[(&readyQueue)->items[i].ptr];
            snprintf(ready_value2, sizeof(ready_value2), "%d", clockcycles - arrival_tmp);

            // Now create pointers array to these local buffers
            const char *ready_values[3] = {ready_value0, ready_value1, ready_value2};

            update_queue_process(&ready_queue, i, ready_values);
        }
    }
}

void update()
{
    // printf("Updating\n");

    // Updating Clock Cycle
    char *new_text = g_strdup_printf("%d", clockcycles);
    gtk_label_set_text(GTK_LABEL(label_clock_value), new_text);
    g_free(new_text);

    // Updating PCB
    update_pcb();

    // updating ready queue
    update_queue((*readyQueue), ready_queue);
    // // updating blocking queues
    // update_queue(&BlockingQueuesNotPtrs[0], file_queue);
    // update_queue(BlockingQueuesNotPtrs[1], userinput_queue);
    // update_queue(BlockingQueuesNotPtrs[2], useroutput_queue);

    // // // updating memory viewer
    // update_queue(*MLFQ_queues[0], ready_queue_q1);
    // update_queue(*MLFQ_queues[1], ready_queue_q2);
    // update_queue(*MLFQ_queues[2], ready_queue_q3);
    // update_queue(*MLFQ_queues[3], ready_queue_rr);
}

void intialize_dashboard()
{
    PopulateMemory();
    update_pcb();
    // updating ready queue
}

void auto_execution(GtkWidget *widget, gpointer user_data)
{
    g_print("Auto execution button clicked.\n");
    stepper = false;
    scheduler();
}

void step_execution(GtkWidget *widget, gpointer user_data)
{
    g_print("Step execution button clicked.\n");
    stepper = true;
    scheduler();
}

void reset_execution(GtkWidget *widget, gpointer user_data)
{
    g_print("Reset execution button clicked.\n");
    stepper = false;
    alreadyRunning = false;
    reset_all();
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