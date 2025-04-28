#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <stdio.h>
#include "program.h"
#include <sys/stat.h>
#include "shared_functions.h"

#define MAX_STRING_LENGTH 100
// Global Variables
int total_processes = 0;
int curr_clock_cycle = 0;
char *current_scheduler = NULL; // Global variable to store the current scheduler type
int RR_quantum = 0;
double arrival_time = 0.0;     // Global variable to store arrival time
gchar *last_user_input = NULL; // Global variable
GtkApplication *application = NULL;
program *programList;
GtkWidget *auto_button;
GtkWidget *step_button;
GtkWidget *reset_button;

// Tables
GtkListStore *process_list_store;
GtkListStore *mutex_status_store;
GtkListStore *memory_viewer_store;
GtkListStore *log_store; // Execution Log
GtkListStore *process_instructions_store;
GtkTreeIter current_iter_instruction_log;

// Global labels for overview grid
GtkWidget *label_total_procs_value;
GtkWidget *label_clock_value;
GtkWidget *label_scheduler_value;

GtkWidget *quantum_input = NULL;
GtkWidget *quantum_button_box = NULL;
GtkWidget *dropdown = NULL;
GtkWidget *main_window = NULL;
// GtkWidget *popup_window = NULL;

gchar *quantum_input_text = NULL; // Pointer to store the quantum input text

// Add a global flag to track if current_iter_instruction_log is valid
gboolean current_iter_valid = FALSE;

typedef struct
{
    GtkWidget *container;
    GtkWidget ***value_labels; // 2D array: [col][row]
    int num_data_columns;      // Fixed at 4
    int num_attributes;        // Number of attributes (e.g., 3 for PID, Instruction, Time)
} DynamicQueueWidget;

// New global data structure for process tracking
typedef struct
{
    int pid;             // Process ID
    double arrival_time; // Arrival time
    char *filename;      // Program file path
} ProcessInfo;
// Global list to store processes
GList *process_list = NULL;

// Modified SetButtonData to include filename
typedef struct
{
    GtkWidget *entry;
    GtkWidget *popup_window;
    char *filename; // Add filename to pass to callback
} SetButtonData;

// Forward declarations of scheduler functions
// backend
void fcfs_scheduler();
void rr_scheduler();
void mlfq_scheduler();

char *filenames;

const char *get_filename_from_path(const char *path)
{
    const char *filename = strrchr(path, '/'); // Find last '/'
    if (filename)
    {
        return filename + 1; // Move past the '/' character
    }
    else
    {
        return path; // No '/' found, path is just the filename
    }
}

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
    programList = malloc(count * sizeof(program));
    return count;
}

// backend
void fcfs_scheduler()
{
    g_print("Starting FCFS Scheduler\n");
}

void rr_scheduler()
{
    g_print("Starting Round Robin Scheduler with quantum level: %d\n", RR_quantum);
}

void mlfq_scheduler()
{
    g_print("Starting MLFQ Scheduler\n");
}

DynamicQueueWidget create_dynamic_queue(const char *queue_title, const char **headers, int num_attributes)
{
    GtkWidget *queue_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    DynamicQueueWidget result = {0};
    result.num_data_columns = 4; // Fixed to match your design
    result.num_attributes = num_attributes;

    // Allocate memory for value_labels: 4 columns x num_attributes rows
    result.value_labels = malloc(sizeof(GtkWidget **) * result.num_data_columns);
    for (int col = 0; col < result.num_data_columns; col++)
    {
        result.value_labels[col] = malloc(sizeof(GtkWidget *) * num_attributes);
        for (int row = 0; row < num_attributes; row++)
        {
            result.value_labels[col][row] = NULL;
        }
    }

    result.container = queue_box;

    // Queue title
    GtkWidget *label_title = gtk_label_new(queue_title);
    char *markup_str = g_strdup_printf("<span font='10'><b>%s</b></span>", queue_title);
    if (markup_str)
    {
        gtk_label_set_markup(GTK_LABEL(label_title), markup_str);
        g_free(markup_str);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(label_title), queue_title);
    }
    gtk_widget_set_halign(label_title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(queue_box), label_title, FALSE, FALSE, 0);

    // Row container
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    // Headers column
    GtkWidget *label_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    for (int i = 0; i < num_attributes; i++)
    {
        const char *header_text = headers[i] ? headers[i] : "";
        GtkWidget *header_label = gtk_label_new(header_text);
        gtk_widget_set_name(header_label, "queue-header-label");
        gtk_box_pack_start(GTK_BOX(label_column), header_label, FALSE, FALSE, 0);
    }

    // Pack the headers column
    gtk_box_pack_start(GTK_BOX(row), label_column, FALSE, FALSE, 5);
    GtkWidget *sep1 = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(row), sep1, FALSE, TRUE, 0);

    // Data columns (4 columns, initially empty)
    for (int col = 0; col < result.num_data_columns; col++)
    {
        GtkWidget *data_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
        gtk_widget_set_name(data_column, col == 0 ? "queue-data-label" : "queue-empty-column");
        for (int row = 0; row < num_attributes; row++)
        {
            result.value_labels[col][row] = gtk_label_new("");
            gtk_widget_set_name(result.value_labels[col][row], "queue-data-label");
            gtk_box_pack_start(GTK_BOX(data_column), result.value_labels[col][row], FALSE, FALSE, 0);
        }
        gtk_box_pack_start(GTK_BOX(row), data_column, FALSE, FALSE, 5);
        if (col < result.num_data_columns - 1)
        {
            GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
            gtk_box_pack_start(GTK_BOX(row), sep, FALSE, TRUE, 0);
        }
    }

    // Pack row into the main vertical box
    gtk_box_pack_start(GTK_BOX(queue_box), row, FALSE, FALSE, 0);

    // Set widget names for CSS
    gtk_widget_set_name(queue_box, "queue-container");
    gtk_widget_set_name(label_title, "queue-title");
    gtk_widget_set_name(row, "queue-row");

    return result;
}
void update_queue_process(DynamicQueueWidget *queue, int col, const char **values)
{
    if (col < 0 || col >= queue->num_data_columns)
        return;
    for (int row = 0; row < queue->num_attributes; row++)
    {
        const char *value = values[row] ? values[row] : "";
        gtk_label_set_text(GTK_LABEL(queue->value_labels[col][row]), value);
    }
}
// Free the queue widget and its labels
void free_dynamic_queue(DynamicQueueWidget *queue)
{
    if (queue->value_labels)
    {
        for (int col = 0; col < queue->num_data_columns; col++)
        {
            for (int row = 0; row < queue->num_attributes; row++)
            {
                if (queue->value_labels[col][row])
                {
                    gtk_widget_destroy(queue->value_labels[col][row]);
                }
            }
            free(queue->value_labels[col]);
        }
        free(queue->value_labels);
    }
    gtk_widget_destroy(queue->container);
}

// Creating the overview grid
GtkWidget *create_overview_grid()
{
    GtkWidget *overview_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(overview_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(overview_grid), 10);

    // Create labels for processes
    GtkWidget *label_total_procs = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_total_procs), "<span font='10'><b>Total Processes: </b></span>");
    char *total_processes_str = g_strdup_printf("%d", total_processes);
    label_total_procs_value = gtk_label_new(total_processes_str); // Assign to global variable
    g_free(total_processes_str);

    // Create labels for clock cycle
    GtkWidget *label_clock = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_clock), "<span font='10'><b>Clock Cycle: </b></span>");
    char *curr_clock_cycle_str = g_strdup_printf("%d", curr_clock_cycle);
    label_clock_value = gtk_label_new(curr_clock_cycle_str); // Assign to global variable
    g_free(curr_clock_cycle_str);

    // Create labels for scheduling algorithm
    GtkWidget *label_scheduler = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_scheduler), "<span font='10'><b>Active Scheduling Algorithm: </b></span>");
    const char *scheduler_display = current_scheduler ? current_scheduler : "None";
    label_scheduler_value = gtk_label_new(scheduler_display); // Assign to global variable

    // Create spacers using empty labels
    GtkWidget *spacer_left = gtk_label_new(NULL);
    GtkWidget *spacer_right = gtk_label_new(NULL);

    // Allow spacers to expand
    gtk_widget_set_hexpand(spacer_left, TRUE);
    gtk_widget_set_hexpand(spacer_right, TRUE);

    // Attach widgets to the grid
    gtk_grid_attach(GTK_GRID(overview_grid), label_total_procs, 0, 0, 1, 1);
    gtk_widget_set_halign(label_total_procs, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(overview_grid), label_total_procs_value, 1, 0, 1, 1);
    gtk_widget_set_halign(label_total_procs_value, GTK_ALIGN_START);

    gtk_grid_attach(GTK_GRID(overview_grid), spacer_left, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(overview_grid), label_clock, 3, 0, 1, 1);
    gtk_widget_set_halign(label_clock, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(overview_grid), label_clock_value, 4, 0, 1, 1);
    gtk_widget_set_halign(label_clock_value, GTK_ALIGN_CENTER);

    gtk_grid_attach(GTK_GRID(overview_grid), spacer_right, 5, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(overview_grid), label_scheduler, 6, 0, 1, 1);
    gtk_widget_set_halign(label_scheduler, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(overview_grid), label_scheduler_value, 7, 0, 1, 1);
    gtk_widget_set_halign(label_scheduler_value, GTK_ALIGN_END);

    gtk_widget_set_margin_start(overview_grid, 100);
    gtk_widget_set_margin_end(overview_grid, 100);
    return overview_grid;
}

// Creating labelled treeview
GtkWidget *create_labeled_treeview(const gchar *label_text, const gchar *column_names[], int num_columns, GtkListStore **store_out)
{

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *label = gtk_label_new(NULL);
    char *markup_str;
    if (strcmp(label_text, "Log & Console Panel") == 0)
    {
        markup_str = g_strdup_printf("<span font='10'><b>Log &amp; Console Panel</b></span>");
    }
    else
    {
        markup_str = g_strdup_printf("<span font='10'><b>%s</b></span>", label_text);
    }
    gtk_label_set_markup(GTK_LABEL(label), markup_str);
    g_free(markup_str);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(label, 5);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    // Only create a new store if it hasn't been initialized
    if (*store_out == NULL)
    {
        GType *types = g_new(GType, num_columns);
        for (int i = 0; i < num_columns; i++)
        {
            types[i] = G_TYPE_STRING;
        }
        *store_out = gtk_list_store_newv(num_columns, types);
        g_free(types);
    }
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(*store_out));
    for (int i = 0; i < num_columns; i++)
    {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(column_names[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    }
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), tree_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    return vbox;
}

// Creating method for labelled frames
GtkWidget *create_labeled_frame(const gchar *label_text)
{
    GtkWidget *frame = gtk_frame_new(NULL); // No built-in title
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), label_text);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5); // Center the label inside the border
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
    gtk_widget_set_size_request(frame, 1000, 200);
    return frame;
}
// show or hide the quantum input field+button based on the selected algorithm
void on_dropdown_changed(GtkComboBox *combo, gpointer user_data)
{
    const gchar *selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (quantum_button_box && GTK_IS_WIDGET(quantum_button_box))
    {
        if (g_strcmp0(selected, "Round Robin") == 0)
        {
            gtk_widget_show_all(quantum_button_box);
        }
        else
        {
            gtk_widget_hide(quantum_button_box);
        }
    }
    else
    {
        g_warning("quantum_button_box is not a valid widget");
    }
    g_free((gchar *)selected);
}
// Button click handler to set quantum level from the entry
void on_set_quantum_level_button_clicked(GtkWidget *widget, gpointer user_data)
{
    quantum_input_text = gtk_entry_get_text(GTK_ENTRY(quantum_input));
    if (quantum_input_text && *quantum_input_text != '\0')
    {
        RR_quantum = atoi(quantum_input_text);
        g_print("Quantum level set to: %d\n", RR_quantum);
    }
    else
    {
        g_print("Please enter a valid quantum level.\n");
    }
}
// Creates the dropdown + conditional quantum_input/button
GtkWidget *create_dropdown_with_quantum_input(GtkWidget **dropdown_out)
{
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_halign(main_hbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(main_hbox, GTK_ALIGN_CENTER);
    // Dropdown menu
    GtkWidget *dropdown = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dropdown), "First Come First Serve");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dropdown), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dropdown), "Multilevel Feedback Queue");
    gtk_combo_box_set_active(GTK_COMBO_BOX(dropdown), 0);
    gtk_box_pack_start(GTK_BOX(main_hbox), dropdown, FALSE, FALSE, 0);
    // Store dropdown in the output parameter
    if (dropdown_out)
    {
        *dropdown_out = dropdown;
    }
    quantum_button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_visible(quantum_button_box, FALSE);

    quantum_input = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(quantum_input), "Please enter the quantum level");
    gtk_widget_set_size_request(quantum_input, 230, 35); // Consistent height
    gtk_box_pack_start(GTK_BOX(quantum_button_box), quantum_input, FALSE, FALSE, 0);

    GtkWidget *set_button = gtk_button_new_with_label("Set");
    gtk_box_pack_start(GTK_BOX(quantum_button_box), set_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_hbox), quantum_button_box, FALSE, FALSE, 0);

    g_signal_connect(dropdown, "changed", G_CALLBACK(on_dropdown_changed), NULL);
    g_signal_connect(set_button, "clicked", G_CALLBACK(on_set_quantum_level_button_clicked), NULL);

    return main_hbox;
}

// Scheduler vbox
GtkWidget *create_scheduler_vbox()
{

    GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    // Auto Button
    auto_button = gtk_button_new_with_label("Auto Execution");
    gtk_box_pack_start(GTK_BOX(button_hbox), auto_button, TRUE, TRUE, 0);
    g_signal_connect(G_OBJECT(auto_button), "clicked", G_CALLBACK(auto_execution), NULL);

    // Step Button
    step_button = gtk_button_new_with_label("Step");
    gtk_box_pack_start(GTK_BOX(button_hbox), step_button, TRUE, TRUE, 0);
    // Reset Button
    reset_button = gtk_button_new_with_label("Reset");
    gtk_box_pack_start(GTK_BOX(button_hbox), reset_button, TRUE, TRUE, 0);
    return button_hbox;
}

// Resource Vbox
GtkWidget *create_resource_vbox()
{
    GtkWidget *resource_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(resource_vbox), 10); // Inner padding for nicer look
    // Mutex Status TreeView
    const gchar *mutex_column_names[] = {"Mutex Name", "Process Owning", "Process Waiting"};
    GtkWidget *mutex_status_treeview = create_labeled_treeview("Mutex Status", mutex_column_names, 3, &mutex_status_store);

    // Add mutex status to the resource VBox
    gtk_box_pack_start(GTK_BOX(resource_vbox), mutex_status_treeview, TRUE, TRUE, 0);

    return resource_vbox;
}
// Memory Viewer
GtkWidget *create_memory_viewer()
{
    const gchar *memory_viewer_columns[] = {"Memory Address", "Data/Instruction"};
    GtkWidget *memory_viewer = create_labeled_treeview("Memory Viewer", memory_viewer_columns, 2, &memory_viewer_store);
    return memory_viewer;
}

// Log & Console Panel
GtkWidget *create_log_console_panel()
{
    GtkWidget *log_console_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(log_console_vbox), 10); // Inner padding for nicer look

    const gchar *log_console_columns[] = {"Instruction", "Process ID", "System Reaction"};
    GtkWidget *log_console_treeview = create_labeled_treeview("Execution Log", log_console_columns, 3, &log_store);
    // Add log console to the log console VBox
    gtk_box_pack_start(GTK_BOX(log_console_vbox), log_console_treeview, TRUE, TRUE, 0);
    // Add label called Event Messages:
    GtkWidget *label_event_messages = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_event_messages), "<span font='10'><b>Event Messages:</b></span>");
    gtk_box_pack_start(GTK_BOX(log_console_vbox), label_event_messages, FALSE, FALSE, 0);
    return log_console_vbox;
}

// Memory+Log
GtkWidget *create_memory_log_hbox()
{
    GtkWidget *memory_log_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(memory_log_hbox), 10); // Inner padding for nicer look

    // Memory Viewer
    GtkWidget *memory_viewer = create_memory_viewer();
    gtk_box_pack_start(GTK_BOX(memory_log_hbox), memory_viewer, TRUE, TRUE, 0);

    // Log & Console Panel
    GtkWidget *log_console_vbox = create_log_console_panel();
    gtk_box_pack_start(GTK_BOX(memory_log_hbox), log_console_vbox, TRUE, TRUE, 0);

    return memory_log_hbox;
}

// Process Configuration
//  Button click handler to set arrival_time from the entry
void on_set_arrival_time_clicked(GtkButton *button, gpointer user_data)
{
    SetButtonData *data = (SetButtonData *)user_data;
    GtkWidget *entry = data->entry;
    GtkWidget *popup_window = data->popup_window;
    char *filename = data->filename;

    // Process the arrival time
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (text && *text != '\0')
    {
        char *endptr;
        double value = g_strtod(text, &endptr);
        if (endptr != text && *endptr == '\0')
        {
            arrival_time = value;
            g_print("Arrival time set to: %.2f\n", arrival_time);
            printf("Filename: %s\n", filename);
            char *only_filename = get_filename_from_path(filename);
            programList[total_processes].arrivalTime = arrival_time;
            strcpy(programList[total_processes].programName, only_filename);

            // Increment total processes
            total_processes++;

            // Create new process info
            ProcessInfo *process = g_new(ProcessInfo, 1);
            process->pid = total_processes;
            process->arrival_time = arrival_time;
            process->filename = g_strdup(filename);

            // Append to process list
            process_list = g_list_append(process_list, process);

            // Add process to process_list_store (only if initialized)
            if (process_list_store && GTK_IS_LIST_STORE(process_list_store))
            {
                gtk_list_store_insert_with_values(process_list_store, NULL, -1,
                                                  0, g_strdup_printf("%d", process->pid),
                                                  1, "Ready",
                                                  2, "Normal",
                                                  3, "TBD",
                                                  4, "0",
                                                  -1);
            }

            // g_print("Process %d added with arrival time %.2f and file %s\n",
            //         process->pid, process->arrival_time, process->filename);
        }
        else
        {
            g_print("Invalid input for arrival time.\n");
        }
    }
    else
    {
        g_print("No arrival time entered.\n");
    }

    // Close the popup window
    gtk_window_close(GTK_WINDOW(popup_window));
}
// Modified callback to free SetButtonData
void on_popup_destroy(GtkWidget *widget, gpointer user_data)
{
    SetButtonData *data = (SetButtonData *)user_data;
    if (data->filename)
        g_free(data->filename);
    g_free(data);
}

// Modified function to create the process config HBox
GtkWidget *create_process_config_hbox(GtkWidget *popup_window, const char *filename)
{
    GtkWidget *process_config_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(process_config_hbox), 5);

    // Arrival Time Label
    GtkWidget *process_arrival_time_label = gtk_label_new("Arrival Time:");
    gtk_widget_set_valign(process_arrival_time_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(process_config_hbox), process_arrival_time_label, FALSE, FALSE, 0);

    // Entry Field
    GtkWidget *arrival_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(arrival_entry), "Please enter the arrival time");
    gtk_widget_set_size_request(arrival_entry, 230, 35);
    gtk_widget_set_valign(arrival_entry, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(process_config_hbox), arrival_entry, FALSE, FALSE, 0);

    // Set Button
    GtkWidget *set_button = gtk_button_new_with_label("Set");
    gtk_widget_set_size_request(set_button, 80, 35);
    gtk_widget_set_valign(set_button, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(process_config_hbox), set_button, FALSE, FALSE, 0);

    // Pass entry, popup_window, and filename to the callback
    SetButtonData *data = g_new(SetButtonData, 1);
    data->entry = arrival_entry;
    data->popup_window = popup_window;
    data->filename = g_strdup(filename);
    g_signal_connect(G_OBJECT(set_button), "clicked", G_CALLBACK(on_set_arrival_time_clicked), data);

    // Free data when the popup is destroyed
    g_signal_connect(G_OBJECT(popup_window), "destroy", G_CALLBACK(on_popup_destroy), data);

    return process_config_hbox;
}
// Changing current instr (for backend)
void append_instruction_log(const gchar *instruction, const gchar *pid, const gchar *reaction)
{
    // If there's a valid current row, clean it up before updating
    if (current_iter_valid && gtk_list_store_iter_is_valid(log_store, &current_iter_instruction_log))
    {
        gchar *old_instr = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(log_store), &current_iter_instruction_log, 0, &old_instr, -1);

        if (old_instr)
        {
            gchar *cleaned = g_strdup(old_instr);
            gchar *original = cleaned;

            if (g_str_has_prefix(cleaned, "->"))
                cleaned += 3;

            gchar *pos = g_strrstr(cleaned, " (Current)");
            if (pos)
                *pos = '\0';

            gtk_list_store_set(log_store, &current_iter_instruction_log, 0, cleaned, -1);
            g_free(original);
            g_free(old_instr);
        }
    }

    // Append a new row
    GtkTreeIter iter;
    gtk_list_store_append(log_store, &iter);

    // Format new current instruction
    gchar *marked_instruction = g_strdup_printf("-> %s (Current)", instruction);

    gtk_list_store_set(log_store, &iter,
                       0, marked_instruction,
                       1, pid,
                       2, reaction,
                       -1);

    current_iter_instruction_log = iter;
    current_iter_valid = TRUE; // Mark as valid
    g_free(marked_instruction);
}

/*
// Execution HBox
GtkWidget *create_execution_hbox()
{
    GtkWidget *execution_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(execution_hbox), 10);
    // Buttons
    GtkWidget *steps_button = gtk_button_new_with_label("Step-By-Step Execution");
    gtk_box_pack_start(GTK_BOX(execution_hbox), steps_button, TRUE, TRUE, 0);
    GtkWidget *auto_button = gtk_button_new_with_label("Auto Execution");
    gtk_box_pack_start(GTK_BOX(execution_hbox), auto_button, TRUE, TRUE, 0);

    return execution_hbox;
}*/
// css
void load_styles()
{
    GtkCssProvider *provider = gtk_css_provider_new();

    if (!gtk_css_provider_load_from_path(provider, "style.css", NULL))
    {
        g_warning("Failed to load CSS file");
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}
// Callback for Enter key (activate signal)
static void on_entry_activate(GtkEntry *entry, gpointer user_data)
{
    gchar **result = (gchar **)user_data;
    const gchar *text = gtk_entry_get_text(entry);
    if (text && *text != '\0')
    {
        *result = g_strdup(text); // Save input to result
    }
}

// Function to create a popup dialog with an input field
gchar *create_input_dialog(GtkWindow *parent)
{
    // Create a modal dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "User Input",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel",
        GTK_RESPONSE_CANCEL,
        "_OK",
        GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);

    // Get the content area
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    // Add a label with instructions
    GtkWidget *label = gtk_label_new("Enter a value and press Enter:");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);

    // Add an entry field
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter value");
    gtk_widget_set_size_request(entry, 200, 35);
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);

    // Apply CSS classes for styling (remove if not using style.css)
    gtk_style_context_add_class(gtk_widget_get_style_context(dialog), "dialog");
    gtk_style_context_add_class(gtk_widget_get_style_context(entry), "entry");
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "label");

    // Variable to store the input
    gchar *result = NULL;

    // Connect Enter key signal
    g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), &result);
    g_signal_connect_swapped(entry, "activate", G_CALLBACK(gtk_dialog_response), dialog);

    // Show all widgets
    gtk_widget_show_all(content_area);

    // Run the dialog
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response != GTK_RESPONSE_OK)
    {
        g_free(result); // Free if set but canceled
        result = NULL;
    }

    // Destroy the dialog
    gtk_widget_destroy(dialog);
    return result; // Return value for saving in last_user_input
}

// Callback for a button to trigger the input dialog
void on_input_dialog_clicked(GtkButton *button, gpointer user_data)
{
    GtkWindow *parent = GTK_WINDOW(user_data);
    // Call the dialog and save the return value to global
    gchar *user_input = create_input_dialog(parent);
    if (user_input)
    {
        if (last_user_input)
            g_free(last_user_input);            // Free previous value
        last_user_input = g_strdup(user_input); // Save to global variable
        g_print("User input saved in last_user_input: %s\n", last_user_input);
        g_free(user_input); // Free the temporary return value
    }
    else
    {
        g_print("No input or dialog canceled\n");
    }
}

GtkWidget *create_fcfs_dashboard()
{
    GtkWidget *dashboard_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // Overview grid
    GtkWidget *overview_grid = create_overview_grid();
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), overview_grid, FALSE, FALSE, 0);

    // Process List
    const gchar *process_list_column_names[] = {"Process ID", "State", "Priority", "Memory Boundaries", "Program Counter"};
    GtkWidget *process_list_treeview = create_labeled_treeview("Process List", process_list_column_names, 5, &process_list_store);
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), process_list_treeview, FALSE, FALSE, 0);

    // Horizontal box to hold ready queue and blocking queues
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), hbox, TRUE, TRUE, 0);

    // Ready Queue
    const char *ready_headers[] = {"Process ID", "Instruction", "Time in Queue"};
    DynamicQueueWidget ready_queue = create_dynamic_queue("Ready Queue", ready_headers, 3);
    const char *ready_values[] = {"1", "Load", "5s"};
    update_queue_process(&ready_queue, 0, ready_values);
    gtk_box_pack_start(GTK_BOX(hbox), ready_queue.container, TRUE, TRUE, 0);

    // Right side VBox for blocking queues
    GtkWidget *blocking_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_box_pack_start(GTK_BOX(hbox), blocking_vbox, TRUE, TRUE, 0);

    // Blocking queues
    const char *blocking_headers[] = {"Process ID", "Instruction", "Time in Queue"};

    // Dummy data for blocking queues
    const char *userinput_values[] = {"2", "Input", "3s"};
    const char *useroutput_values[] = {"3", "Output", "2s"};
    const char *file_values[] = {"4", "File Read", "4s"};

    // User Input Blocking Queue
    DynamicQueueWidget userinput_queue = create_dynamic_queue("User Input Blocking Queue", blocking_headers, 3);
    update_queue_process(&userinput_queue, 0, userinput_values);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), userinput_queue.container, TRUE, TRUE, 0);

    // User Output Blocking Queue
    DynamicQueueWidget useroutput_queue = create_dynamic_queue("User Output Blocking Queue", blocking_headers, 3);
    update_queue_process(&useroutput_queue, 0, useroutput_values);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), useroutput_queue.container, TRUE, TRUE, 0);

    // File Blocking Queue
    DynamicQueueWidget file_queue = create_dynamic_queue("File Blocking Queue", blocking_headers, 3);
    update_queue_process(&file_queue, 0, file_values);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), file_queue.container, TRUE, TRUE, 0);

    return dashboard_vbox;
}

// RR Dashboard
GtkWidget *create_rr_dashboard()
{
    GtkWidget *dashboard_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // Overview grid
    GtkWidget *overview_grid = create_overview_grid();
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), overview_grid, FALSE, FALSE, 0);

    // Process List
    const gchar *process_list_column_names[] = {"Process ID", "State", "Priority", "Memory Boundaries", "Program Counter"};
    GtkWidget *process_list_treeview = create_labeled_treeview("Process List", process_list_column_names, 5, &process_list_store);
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), process_list_treeview, FALSE, FALSE, 0);

    // Horizontal box to hold ready queue and blocking queues
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), hbox, FALSE, FALSE, 0);

    // Ready Queue
    const char *ready_headers[] = {"Process ID", "Instruction", "Time in Queue"};
    const char *ready_values[] = {"1", "Load", "5s"};
    DynamicQueueWidget ready_queue = create_dynamic_queue("Ready Queue", ready_headers, 3);
    update_queue_process(&ready_queue, 0, ready_values);
    gtk_box_pack_start(GTK_BOX(hbox), ready_queue.container, TRUE, TRUE, 0);

    // Right side VBox for blocking queues
    GtkWidget *blocking_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_box_pack_start(GTK_BOX(hbox), blocking_vbox, TRUE, TRUE, 0);

    // Blocking queues
    const char *blocking_headers[] = {"Process ID", "Instruction", "Time in Queue"};

    // Dummy data for blocking queues
    const char *userinput_values[] = {"2", "Input", "3s"};
    const char *useroutput_values[] = {"3", "Output", "2s"};
    const char *file_values[] = {"4", "File Read", "4s"};

    // User Input Blocking Queue
    DynamicQueueWidget userinput_queue = create_dynamic_queue("User Input Blocking Queue", blocking_headers, 3);
    update_queue_process(&userinput_queue, 0, userinput_values);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), userinput_queue.container, TRUE, TRUE, 0);

    // User Output Blocking Queue
    DynamicQueueWidget useroutput_queue = create_dynamic_queue("User Output Blocking Queue", blocking_headers, 3);
    update_queue_process(&useroutput_queue, 0, useroutput_values);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), useroutput_queue.container, TRUE, TRUE, 0);

    // File Blocking Queue
    DynamicQueueWidget file_queue = create_dynamic_queue("File Blocking Queue", blocking_headers, 3);
    update_queue_process(&file_queue, 0, file_values);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), file_queue.container, TRUE, TRUE, 0);

    return dashboard_vbox;
}

GtkWidget *create_mlfq_dashboard()
{
    GtkWidget *dashboard_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // Overview grid
    GtkWidget *overview_grid = create_overview_grid();
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), overview_grid, TRUE, TRUE, 0);

    // Process List
    const gchar *process_list_column_names[] = {"Process ID", "State", "Priority", "Memory Boundaries", "Program Counter"};
    GtkWidget *process_list_treeview = create_labeled_treeview("Process List", process_list_column_names, 5, &process_list_store);
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), process_list_treeview, TRUE, TRUE, 0);

    // Horizontal box to hold ready queues (left) and blocking queues (right)
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);

    // Vertical box to hold ready queues (left side of hbox)
    GtkWidget *ready_queue_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // Ready Queues (Q=1, Q=2, Q=4)
    const char *ready_headers[] = {"Process ID", "Instruction", "Time in Queue"};

    DynamicQueueWidget ready_queue_q1 = create_dynamic_queue("Ready Queue (Q=1)", ready_headers, 3);
    gtk_box_pack_start(GTK_BOX(ready_queue_vbox), ready_queue_q1.container, TRUE, TRUE, 0);

    DynamicQueueWidget ready_queue_q2 = create_dynamic_queue("Ready Queue (Q=2)", ready_headers, 3);
    // No initial values for Q=2
    gtk_box_pack_start(GTK_BOX(ready_queue_vbox), ready_queue_q2.container, TRUE, TRUE, 0);

    DynamicQueueWidget ready_queue_q3 = create_dynamic_queue("Ready Queue (Q=4)", ready_headers, 3);
    // No initial values for Q=4
    gtk_box_pack_start(GTK_BOX(ready_queue_vbox), ready_queue_q3.container, TRUE, TRUE, 0);

    DynamicQueueWidget ready_queue_rr = create_dynamic_queue("Ready Queue (Round Robin)", ready_headers, 3);
    gtk_box_pack_start(GTK_BOX(ready_queue_vbox), ready_queue_rr.container, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), ready_queue_vbox, TRUE, TRUE, 0);

    // Right side VBox for blocking queues
    GtkWidget *blocking_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // Blocking queues
    const char *blocking_headers[] = {"Process ID", "Instruction", "Time in Queue"};

    // User Input Blocking Queue
    DynamicQueueWidget userinput_queue = create_dynamic_queue("User Input Blocking Queue", blocking_headers, 3);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), userinput_queue.container, TRUE, TRUE, 0);

    // User Output Blocking Queue
    DynamicQueueWidget useroutput_queue = create_dynamic_queue("User Output Blocking Queue", blocking_headers, 3);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), useroutput_queue.container, TRUE, TRUE, 0);

    // File Blocking Queue
    DynamicQueueWidget file_queue = create_dynamic_queue("File Blocking Queue", blocking_headers, 3);
    gtk_box_pack_start(GTK_BOX(blocking_vbox), file_queue.container, TRUE, TRUE, 0);

    // Pack blocking_vbox on the right side of hbox
    gtk_box_pack_start(GTK_BOX(hbox), blocking_vbox, TRUE, TRUE, 0);

    // Pack hbox into dashboard_vbox
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), hbox, TRUE, TRUE, 0);

    return dashboard_vbox;
}

GtkWidget *create_error_dashboard()
{
    // Create a vertical box to hold the error message
    GtkWidget *dashboard_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_name(dashboard_vbox, "error-dashboard");

    // Create a label with the error message
    GtkWidget *error_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(error_label),
                         "<span font='14'><b>Please select a valid scheduler to proceed.</b></span>");
    gtk_widget_set_halign(error_label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(error_label, GTK_ALIGN_CENTER);
    gtk_widget_set_name(error_label, "error-label");

    // Pack the label into the box
    gtk_box_pack_start(GTK_BOX(dashboard_vbox), error_label, TRUE, TRUE, 0);

    // Add some margin for better appearance
    gtk_widget_set_margin_start(dashboard_vbox, 20);
    gtk_widget_set_margin_end(dashboard_vbox, 20);
    gtk_widget_set_margin_top(dashboard_vbox, 20);
    gtk_widget_set_margin_bottom(dashboard_vbox, 20);

    return dashboard_vbox;
}

static void setup_main_window(GtkWidget *window)
{
    load_styles();

    gtk_window_set_default_size(GTK_WINDOW(window), 1500, 950);
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);

    GtkWidget *main_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(main_container), 10);
    gtk_container_add(GTK_CONTAINER(scrolled_window), main_container);
    gtk_widget_set_margin_start(main_container, 20);
    gtk_widget_set_margin_end(main_container, 20);

    // Scheduler Buttons
    GtkWidget *scheduler_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(scheduler_frame), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(main_container), scheduler_frame, FALSE, FALSE, 0);
    gtk_widget_set_size_request(scheduler_frame, -1, 50);
    GtkWidget *scheduler_vbox = create_scheduler_vbox();
    gtk_container_add(GTK_CONTAINER(scheduler_frame), scheduler_vbox);

    // Main Dashboard
    GtkWidget *dashboard_frame = create_labeled_frame("<span font='12'><b>Main Dashboard</b></span>");
    gtk_box_pack_start(GTK_BOX(main_container), dashboard_frame, FALSE, FALSE, 0);
    GtkWidget *dashboard_vbox;
    if (g_strcmp0(current_scheduler, "First Come First Serve") == 0)
    {
        dashboard_vbox = create_fcfs_dashboard();
    }
    else if (g_strcmp0(current_scheduler, "Round Robin") == 0)
    {
        dashboard_vbox = create_rr_dashboard();
    }
    else if (g_strcmp0(current_scheduler, "Multilevel Feedback Queue") == 0)
    {
        dashboard_vbox = create_mlfq_dashboard();
    }
    else
    {
        dashboard_vbox = create_error_dashboard();
    }
    gtk_container_add(GTK_CONTAINER(dashboard_frame), dashboard_vbox);

    // Resource Management
    GtkWidget *resource_frame = create_labeled_frame("<span font='12'><b>Resource Management Panel</b></span>");
    gtk_box_pack_start(GTK_BOX(main_container), resource_frame, FALSE, FALSE, 0);
    GtkWidget *resource_vbox = create_resource_vbox();
    gtk_container_add(GTK_CONTAINER(resource_frame), resource_vbox);

    // Memory Log and Console Panel
    GtkWidget *memory_log_frame = create_labeled_frame("<span font='12'><b>Memory and Log &amp; Console Panel</b></span>");
    gtk_box_pack_start(GTK_BOX(main_container), memory_log_frame, FALSE, FALSE, 0);
    GtkWidget *memory_log_hbox = create_memory_log_hbox();
    gtk_container_add(GTK_CONTAINER(memory_log_frame), memory_log_hbox);

    // Execution Frame
    GtkWidget *execution_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(execution_frame), GTK_SHADOW_IN);
    gtk_widget_set_size_request(execution_frame, -1, 50);
    gtk_box_pack_start(GTK_BOX(main_container), execution_frame, FALSE, FALSE, 0);
    // GtkWidget *execution_hbox = create_execution_hbox();
    // gtk_container_add(GTK_CONTAINER(execution_frame), execution_hbox);

    // Update UI with process information
    if (label_total_procs_value && GTK_IS_LABEL(label_total_procs_value))
    {
        char *total_processes_str = g_strdup_printf("%d", total_processes);
        gtk_label_set_text(GTK_LABEL(label_total_procs_value), total_processes_str);
        g_free(total_processes_str);
    }
    else
    {
        g_warning("label_total_procs_value is not a valid GtkLabel");
    }

    // Populate process_list_store with any processes added in the initial window
    if (process_list_store && GTK_IS_LIST_STORE(process_list_store))
    {
        for (GList *iter = process_list; iter != NULL; iter = iter->next)
        {
            ProcessInfo *process = (ProcessInfo *)iter->data;
            gtk_list_store_insert_with_values(process_list_store, NULL, -1,
                                              0, g_strdup_printf("%d", process->pid),
                                              1, "Ready",
                                              2, "Normal",
                                              3, "TBD",
                                              4, "0",
                                              -1);
        }
    }
    else
    {
        g_warning("process_list_store is not a valid GtkListStore");
    }

    // Sample data (optional, remove if not needed)
    gtk_list_store_insert_with_values(process_list_store, NULL, -1,
                                      0, "1",
                                      1, "Running",
                                      2, "High",
                                      3, "0-100",
                                      4, "500",
                                      -1);

    gtk_list_store_insert_with_values(mutex_status_store, NULL, -1,
                                      0, "Mutex1",
                                      1, "8",
                                      2, "9",
                                      -1);

    for (int i = 0; i < 60; i++)
    {
        GtkTreeIter iter;
        gchar *address_str = g_strdup_printf("0x%04X", i);
        gtk_list_store_append(memory_viewer_store, &iter);
        gtk_list_store_set(memory_viewer_store, &iter,
                           0, address_str,
                           1, "",
                           -1);
        g_free(address_str);
    }

    append_instruction_log("LOAD R1, A", "P1", "Acquired user quantum_input");
    append_instruction_log("ADD R2, R1", "P2", "Waiting for file");
    append_instruction_log("STORE R2, B", "P3", "Released user quantum_input");

    gtk_widget_show_all(window);
}

void on_simulate_clicked(GtkButton *button, gpointer user_data)
{
    GtkComboBoxText *dropdown = GTK_COMBO_BOX_TEXT(user_data);
    const gchar *algorithm = gtk_combo_box_text_get_active_text(dropdown);

    // Store selected algorithm in global variable
    if (current_scheduler)
        g_free(current_scheduler); // Free previous if any
    current_scheduler = g_strdup(algorithm);
    // Call the appropriate scheduler function based on the selection
    if (g_strcmp0(current_scheduler, "First Come First Serve") == 0)
    {
        printf("Selected: First Come First Serve\n");
    }
    else if (g_strcmp0(current_scheduler, "Round Robin") == 0)
    {
        printf("Selected: Round Robin\n");
    }
    else if (g_strcmp0(current_scheduler, "Multilevel Feedback Queue") == 0)
    {
        printf("Selected: Multilevel Feedback Queue\n");
    }
    else
    {
        g_print("Unknown algorithm selected.\n");
    }

    // Close initial window
    GtkWindow *initial_window = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(button)));
    gtk_widget_destroy(GTK_WIDGET(initial_window));

    // Create and show main window
    main_window = gtk_application_window_new(application);
    setup_main_window(main_window); // We'll modify your existing activate code
}

// This function will handle the file confirmation and display contents in a new popup window
void handle_file_confirmation(GtkWidget *file_chooser)
{
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
    g_print("File selected: %s\n", filename);

    // Create a new popup window to display the file content
    GtkWidget *popup_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(popup_window), "File Contents");
    gtk_window_set_default_size(GTK_WINDOW(popup_window), 400, 300);

    GtkWidget *popup_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(popup_window), popup_vbox);

    GtkWidget *content_label = gtk_label_new("Instructions:");
    gtk_box_pack_start(GTK_BOX(popup_vbox), content_label, FALSE, FALSE, 5);

    // Validate file
    if (strstr(filename, ".txt") != NULL)
    {

        FILE *file = fopen(filename, "r");
        if (file)
        {
            char line[256];
            while (fgets(line, sizeof(line), file))
            {
                GtkWidget *line_label = gtk_label_new(line);
                gtk_box_pack_start(GTK_BOX(popup_vbox), line_label, FALSE, FALSE, 5);
            }
            fclose(file);

            // Create process config hbox with filename
            GtkWidget *process_config_hbox = create_process_config_hbox(popup_window, filename);
            gtk_widget_set_valign(process_config_hbox, GTK_ALIGN_CENTER);
            gtk_box_pack_start(GTK_BOX(popup_vbox), process_config_hbox, FALSE, FALSE, 0);
        }
        else
        {
            GtkWidget *error_label = gtk_label_new("Error: Unable to open the selected file.");
            gtk_box_pack_start(GTK_BOX(popup_vbox), error_label, FALSE, FALSE, 5);
        }
    }
    else
    {
        GtkWidget *error_label = gtk_label_new("Error: Invalid file selected.");
        gtk_box_pack_start(GTK_BOX(popup_vbox), error_label, FALSE, FALSE, 5);
    }

    g_free(filename);
    gtk_widget_show_all(popup_window);
}

// Optional: Function to free process_list on application exit
void free_process_list()
{
    for (GList *iter = process_list; iter != NULL; iter = iter->next)
    {
        ProcessInfo *process = (ProcessInfo *)iter->data;
        if (process->filename)
            g_free(process->filename);
        g_free(process);
    }
    g_list_free(process_list);
    process_list = NULL;
}

// This function shows the dialog for file selection
void on_add_process_clicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog, *file_chooser;
    gint result;

    // Create the dialog window (this is for selecting the file)
    dialog = gtk_dialog_new_with_buttons("Choose a Program File",
                                         GTK_WINDOW(data),
                                         GTK_DIALOG_MODAL,
                                         "_Cancel", GTK_RESPONSE_REJECT,
                                         "_Confirm", GTK_RESPONSE_ACCEPT,
                                         NULL);

    // Set up file chooser
    file_chooser = gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), "./Programs");
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), filter);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(file_chooser), "./Programs/Program_1.txt");

    // Add file chooser to dialog
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), file_chooser, TRUE, TRUE, 10);
    gtk_widget_show_all(dialog);

    // Run the dialog and wait for the result
    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT)
    {
        // Call the function to handle confirmation and show the file content in a new popup window
        handle_file_confirmation(file_chooser);
    }
    else if (result == GTK_RESPONSE_REJECT)
    {
        printf("File selection was canceled.\n");
    }

    gtk_widget_destroy(dialog); // Close the dialog
}

GtkWidget *create_initial_window()
{
    count_txt_files("./Programs");
    load_styles();

    // Create initial window
    GtkWidget *initial_window = gtk_application_window_new(application);
    gtk_window_set_default_size(GTK_WINDOW(initial_window), 1500, 950);

    // frame
    GtkWidget *initial_frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(initial_window), initial_frame);
    gtk_widget_set_margin_start(initial_frame, 20);
    gtk_widget_set_margin_end(initial_frame, 20);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 30);
    gtk_container_add(GTK_CONTAINER(initial_frame), vbox);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 20);

    GtkWidget *title_label = gtk_label_new("Scheduler Simulation");
    gtk_widget_set_name(title_label, "title-label");

    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);

    GtkWidget *dropdown_label = gtk_label_new("Which scheduler would you like to use for the simulation?");
    gtk_widget_set_name(dropdown_label, "dropdown-label");
    gtk_box_pack_start(GTK_BOX(vbox), dropdown_label, FALSE, FALSE, 0);

    // Dropdown menu
    GtkWidget *dropdown_hbox = create_dropdown_with_quantum_input(&dropdown);
    gtk_box_pack_start(GTK_BOX(vbox), dropdown_hbox, FALSE, FALSE, 0);

    // Process
    GtkWidget *processes_label = gtk_label_new("Which process(es) would you like to add?");
    gtk_widget_set_name(processes_label, "processes-label");
    gtk_box_pack_start(GTK_BOX(vbox), processes_label, FALSE, FALSE, 0);

    // Add Process Button
    GtkWidget *add_button = gtk_button_new_with_label("Add Process");
    gtk_widget_set_size_request(add_button, 120, 40);
    gtk_widget_set_valign(add_button, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), add_button, FALSE, FALSE, 0);
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_process_clicked), initial_window);

    // Simulate button
    GtkWidget *simulate_btn = gtk_button_new_with_label("Start Simulation!");
    g_signal_connect(simulate_btn, "clicked", G_CALLBACK(on_simulate_clicked), dropdown);
    gtk_box_pack_start(GTK_BOX(vbox), simulate_btn, FALSE, FALSE, 0);

    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);

    return initial_window;
}

// Function to activate the GTK application window

// int main(int argc, char *argv[]) {
//     int status;

//     // Initialize process_list_store
//     GType process_types[] = {G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING};
//     process_list_store = gtk_list_store_newv(5, process_types);

//     // Initialize mutex_status_store
//     GType mutex_types[] = {G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING};
//     mutex_status_store = gtk_list_store_newv(3, mutex_types);

//     // Initialize memory_viewer_store
//     GType memory_types[] = {G_TYPE_STRING, G_TYPE_STRING};
//     memory_viewer_store = gtk_list_store_newv(2, memory_types);

//     // Initialize log_store
//     GType log_types[] = {G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING};
//     log_store = gtk_list_store_newv(3, log_types);

//     application = gtk_application_new("simulation.scheduler", G_APPLICATION_DEFAULT_FLAGS);
//     g_signal_connect(application, "activate", G_CALLBACK(activate), NULL);
//     status = g_application_run(G_APPLICATION(application), argc, argv);
//     g_object_unref(application);

//     // Clean up
//     if (process_list_store) {
//         g_object_unref(process_list_store);
//         process_list_store = NULL;
//     }
//     if (mutex_status_store) {
//         g_object_unref(mutex_status_store);
//         mutex_status_store = NULL;
//     }
//     if (memory_viewer_store) {
//         g_object_unref(memory_viewer_store);
//         memory_viewer_store = NULL;
//     }
//     if (log_store) {
//         g_object_unref(log_store);
//         log_store = NULL;
//     }
//     free_process_list();

//     return status;
// }