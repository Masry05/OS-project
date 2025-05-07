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

char *assignInput()
{
    printf("Assigning input\n");
    return create_input_dialog();
}

void printFromToGUI(char *result)
{
    // call a gui function to display the result
    printPopUp(result);
}

void update_pcb()
{
    printf("Updating PCB\n");

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
    printf("completed pcb update\n");
}

void update_queue(MemQueue readyQueue, DynamicQueueWidget ready_queue)
{
    // printf("Before\n");
    for (int i = 0; i < total_processes + 1; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            const char *value = "";
            gtk_label_set_text(GTK_LABEL(ready_queue.value_labels[i][j]), value);
        }
    }
    // printf("after1\n");

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
    // printf("after2\n");
}

void update_memory()
{
    printf("Updating memory\n");
    gtk_list_store_clear(memory_viewer_store);
    for (int i = 0; i < 60; i++)
    {
        GtkTreeIter iter;
        gchar *address_str = g_strdup_printf("0x%04X", i);
        bool empty = true;
        char mul_values[MAX_STRING_LENGTH];
        if (memory[i].value2[0] != '\0')
        {
            strcpy(mul_values, memory[i].value);
            strcat(mul_values, ":");
            strcat(mul_values, memory[i].value2);
            empty = false;
        }
        gtk_list_store_append(memory_viewer_store, &iter);
        gtk_list_store_set(memory_viewer_store, &iter,
                           0, address_str,
                           1, empty ? memory[i].value : mul_values,
                           -1);
        g_free(address_str);
    }
    printf("completed memory update\n");
}

// void append_instruction_log(const gchar *instruction, const gchar *pid, const gchar *reaction)

void updateExecutionLog(const gchar *instruction, const gchar *pid, const gchar *reaction)
{

    append_instruction_log(instruction, pid, reaction);
}

void addEventMessage(char *message)
{
    append_event_message(message);
}

void updateMutex(int program, int resource, bool status)
{
    char *resource_name;
    switch (resource)
    {
    case 0:
        resource_name = "File Access";
        break;
    case 1:
        resource_name = "User Input";
        break;
    case 2:
        resource_name = "User Output";
        break;
    default:
        resource_name = "Unknown";
    }
    char *program_name = g_strdup_printf("Process ID %d", program);

    if (!status)
    {
        gtk_list_store_insert_with_values(mutex_status_store, NULL, -1,
                                          0, resource_name,
                                          1, program_name,
                                          -1);
    }
    else
    {
        GtkTreeIter iter;
        gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(mutex_status_store), &iter);

        while (valid)
        {
            gchar *current_resource_name;
            gtk_tree_model_get(GTK_TREE_MODEL(mutex_status_store), &iter,
                               0, &current_resource_name, // Column 0 contains the resource name
                               -1);

            if (g_strcmp0(current_resource_name, resource_name) == 0)
            {
                // Remove the row if the resource name matches
                valid = gtk_list_store_remove(mutex_status_store, &iter);
            }
            else
            {
                // Move to the next row
                valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(mutex_status_store), &iter);
            }

            g_free(current_resource_name); // Free the allocated string
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
    if (algo != 2)
        update_queue(*readyQueue, ready_queue);
    else
    {
        update_queue(MLFQ_queues_not_ptrs[0], ready_queue_q1);
        update_queue(MLFQ_queues_not_ptrs[1], ready_queue_q2);
        update_queue(MLFQ_queues_not_ptrs[2], ready_queue_q3);
        update_queue(MLFQ_queues_not_ptrs[3], ready_queue_rr);
    }
    // updating blocking queues

    update_queue(BlockingQueuesNotPtrs[0], file_queue);
    update_queue(BlockingQueuesNotPtrs[1], userinput_queue);
    update_queue(BlockingQueuesNotPtrs[2], useroutput_queue);

    // updating memory viewer

    update_memory();
}

void intialize_dashboard()
{
    PopulateMemory();
    update_pcb();
    update_memory();
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
    update();
    gtk_list_store_clear(mutex_status_store);
    gtk_list_store_clear(log_store);
    append_event_message("Execution reset.\n");
    // gtk_list_store_clear(process_instructions_store);
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

void backButtonPressed()
{
    reset_execution(NULL, NULL);
    update_pcb();
    update_memory();
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