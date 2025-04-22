#include <gtk/gtk.h>

//Global Variables
GtkListStore *process_list_store;
GtkListStore *ready_queue_store;
GtkListStore *blocked_queue_store;
GtkListStore *running_process_store;
GtkListStore *mutex_status_store;
GtkListStore *memory_viewer_store;
GtkListStore *log_store;
GtkListStore *process_instructions_store;
GtkTreeIter current_iter_instruction_log; // For marking the current instruction in the log

GtkWidget *quantum_input; 
GtkWidget *quantum_button_box;  // Container for quantum_input + button

int total_processes=0;
int curr_clock_cycle=0;
char *current_scheduler = NULL; // Global variable to store the current scheduler type
int quantum_level = 0;
double arrival_time = 0.0; // Global variable to store arrival time

//Creating the overview grid
GtkWidget* create_overview_grid(){
    GtkWidget* overview_grid= gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(overview_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(overview_grid), 10);

    // Create labels for processes
    GtkWidget *label_total_procs = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_total_procs), "<span font='10'><b>Total Processes: </b></span>");
    char *total_processes_str = g_strdup_printf("%d", total_processes);
    GtkWidget *label_procs_value = gtk_label_new(total_processes_str);

    // Create labels for clock cycle
    GtkWidget *label_clock = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_clock), "<span font='10'><b>Clock Cycle: </b></span>");
    char *curr_clock_cycle_str = g_strdup_printf("%d", curr_clock_cycle);
    GtkWidget *label_clock_value = gtk_label_new(curr_clock_cycle_str);

    // Create labels for scheduling algorithm
    GtkWidget *label_scheduler = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label_scheduler), "<span font='10'><b>Active Scheduling Algorithm: </b></span>");
    if (current_scheduler == NULL) {
        current_scheduler = g_strdup("None");
    }
    GtkWidget *label_scheduler_value = gtk_label_new(current_scheduler);  

        // Create spacers using empty labels
    GtkWidget *spacer_left = gtk_label_new(NULL);
    GtkWidget *spacer_right = gtk_label_new(NULL);

    // Allow spacers to expand
    gtk_widget_set_hexpand(spacer_left, TRUE);
    gtk_widget_set_hexpand(spacer_right, TRUE);

    // Attach widgets to the grid
    gtk_grid_attach(GTK_GRID(overview_grid), label_total_procs, 0, 0, 1, 1);
    gtk_widget_set_halign(label_total_procs, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(overview_grid), label_procs_value, 1, 0, 1, 1);
    gtk_widget_set_halign(label_procs_value, GTK_ALIGN_START);

    // Spacer to push next pair to center
    gtk_grid_attach(GTK_GRID(overview_grid), spacer_left, 2, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(overview_grid), label_clock, 3, 0, 1, 1);
    gtk_widget_set_halign(label_clock, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(overview_grid), label_clock_value, 4, 0, 1, 1);
    gtk_widget_set_halign(label_clock_value, GTK_ALIGN_CENTER);

    // Spacer to push next pair to the right
    gtk_grid_attach(GTK_GRID(overview_grid), spacer_right, 5, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(overview_grid), label_scheduler, 6, 0, 1, 1);
    gtk_widget_set_halign(label_scheduler, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(overview_grid), label_scheduler_value, 7, 0, 1, 1);
    gtk_widget_set_halign(label_scheduler_value, GTK_ALIGN_END);


    // Free dynamically allocated strings after use
    g_free(total_processes_str);
    g_free(curr_clock_cycle_str);
    // Add overview to dashboard vbox
    gtk_widget_set_margin_start(overview_grid, 100);
    gtk_widget_set_margin_end(overview_grid, 100);
    return overview_grid;
}

//Creating labelled treeview
GtkWidget* create_labeled_treeview(const gchar *label_text, const gchar *column_names[], int num_columns, GtkListStore **store_out) {

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *label = gtk_label_new(NULL);
    char *markup_str;
        if (strcmp(label_text, "Log & Console Panel") == 0) {
        markup_str = g_strdup_printf("<span font='10'><b>Log &amp; Console Panel</b></span>");
    } else {
        markup_str = g_strdup_printf("<span font='10'><b>%s</b></span>", label_text);
    }
    gtk_label_set_markup(GTK_LABEL(label), markup_str);
    g_free(markup_str); 
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(label, 5);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0); 

    GType *types = g_new(GType, num_columns);
    for (int i = 0; i < num_columns; i++) {
        types[i] = G_TYPE_STRING;  //could be changed
    }
    *store_out = gtk_list_store_newv(num_columns, types);
    g_free(types);
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(*store_out));
    for (int i = 0; i < num_columns; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(column_names[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    }
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), tree_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    return vbox;
}

//Ready Queue, Blocked Queue, Running Process
static GtkWidget* create_hbox_for_queues() {
    GtkWidget *hbox;

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    //Queues
    const gchar *queue_column_names[] = {"Process ID", "Time in Queue"};
    GtkWidget *ready_queue = create_labeled_treeview("Ready Queue", queue_column_names, 2, &ready_queue_store);
    GtkWidget *blocked_queue = create_labeled_treeview("Blocked Queue", queue_column_names, 2, &blocked_queue_store);
    const gchar *running_process_columns[] = {"Process ID"};
    GtkWidget *running_process = create_labeled_treeview("Running Process", running_process_columns, 1, &running_process_store);

    gtk_box_pack_start(GTK_BOX(hbox), ready_queue, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), blocked_queue, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), running_process, TRUE, TRUE, 0);

    return hbox;
}
//Creating the dashboard
GtkWidget* create_dashboard() {
    GtkWidget* dashboard_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
        // Overview grid
        GtkWidget* overview_grid = create_overview_grid();
        gtk_box_pack_start(GTK_BOX(dashboard_vbox), overview_grid, TRUE, TRUE, 0);
        
        // Process List 
        const gchar *process_list_column_names[] = {"Process ID", "State", "Priority", "Memory Boundaries", "Program Counter"};
        GtkWidget* process_list_treeview = create_labeled_treeview("Process List", process_list_column_names, 5, &process_list_store);
        gtk_box_pack_start(GTK_BOX(dashboard_vbox), process_list_treeview, TRUE, TRUE, 0);
    
        // Ready,Blocked,Running
        GtkWidget *queues_hbox = create_hbox_for_queues();
        gtk_box_pack_start(GTK_BOX(dashboard_vbox), queues_hbox, TRUE, TRUE, 0);
    return dashboard_vbox;
}

//Creating method for labelled frames
GtkWidget* create_labeled_frame(const gchar *label_text) {
    GtkWidget *frame = gtk_frame_new(NULL);  // No built-in title
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), label_text);
    gtk_frame_set_label_widget(GTK_FRAME(frame), label);
    gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);  // Center the label inside the border
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
    gtk_widget_set_size_request(frame, 1000, 200);
    return frame;
}
// show or hide the quantum input field+button based on the selected algorithm
void on_dropdown_changed(GtkComboBox *combo, gpointer user_data) {
    const gchar *selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (g_strcmp0(selected, "Round Robin") == 0) {
        gtk_widget_show_all(quantum_button_box);
    } else {
        gtk_widget_hide(quantum_button_box); 
    }
    g_free((gchar *)selected);
}
// Button click handler to set quantum level from the entry
void on_set_quantum_level_button_clicked(GtkWidget *widget, gpointer user_data) {
    const gchar *quantum_input_text = gtk_entry_get_text(GTK_ENTRY(quantum_input));
    if (quantum_input_text && *quantum_input_text != '\0') {
        quantum_level = atoi(quantum_input_text);
        g_print("Quantum level set to: %d\n", quantum_level);
    } else {
        g_print("Please enter a valid quantum level.\n");
    }
}
// Creates the dropdown + conditional quantum_input/button
GtkWidget* create_dropdown_with_quantum_input() {
    GtkWidget *main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    // Dropdown menu
    GtkWidget *dropdown = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dropdown), "First Come First Serve");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dropdown), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dropdown), "Multilevel Feedback Queue");
    gtk_combo_box_set_active(GTK_COMBO_BOX(dropdown), 0);
    gtk_box_pack_start(GTK_BOX(main_hbox), dropdown, FALSE, FALSE, 0);

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

//Scheduler vbox
GtkWidget* create_scheduler_vbox() {
    GtkWidget* scheduler_vbox= gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(scheduler_vbox), 10); 

    // Scheduler dropdown and entry field
    GtkWidget* drop_down_hbox =  create_dropdown_with_quantum_input();   
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), drop_down_hbox, FALSE, FALSE, 0);

    // HBox for control buttons
    GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    // Start Button
    GtkWidget *start_button = gtk_button_new_with_label("Start");
    gtk_box_pack_start(GTK_BOX(button_hbox), start_button, TRUE, TRUE, 0);

    // Stop Button
    GtkWidget *stop_button = gtk_button_new_with_label("Stop");
    gtk_box_pack_start(GTK_BOX(button_hbox), stop_button, TRUE, TRUE, 0);

    // Reset Button
    GtkWidget *reset_button = gtk_button_new_with_label("Reset");
    gtk_box_pack_start(GTK_BOX(button_hbox), reset_button, TRUE, TRUE, 0);

    // Add the button box to the scheduler panel
    gtk_box_pack_start(GTK_BOX(scheduler_vbox), button_hbox, FALSE, FALSE,0);

    return scheduler_vbox;
}

//Resource Vbox
GtkWidget* create_resource_vbox() {
    GtkWidget* resource_vbox= gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(resource_vbox), 10); // Inner padding for nicer look
    // Mutex Status TreeView
    const gchar *mutex_column_names[] = {"Mutex Name", "Process Owning", "Process Waiting"};
    GtkWidget *mutex_status_treeview = create_labeled_treeview("Mutex Status", mutex_column_names, 3, &mutex_status_store);

    //Add mutex status to the resource VBox
    gtk_box_pack_start(GTK_BOX(resource_vbox), mutex_status_treeview, TRUE, TRUE, 0);

    return resource_vbox;
}
//Memory Viewer
GtkWidget* create_memory_viewer() {
    const gchar *memory_viewer_columns[] = {"Memory Address", "Data/Instruction"};
    GtkWidget *memory_viewer = create_labeled_treeview("Memory Viewer", memory_viewer_columns, 2, &memory_viewer_store);
    return memory_viewer;
}

//Log & Console Panel
GtkWidget* create_log_console_panel() {
    GtkWidget *log_console_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(log_console_vbox), 10); // Inner padding for nicer look

    const gchar *log_console_columns[] = {"Instruction", "Process ID", "System Reaction"};
    GtkWidget *log_console_treeview = create_labeled_treeview("Log & Console Panel", log_console_columns, 3, &log_store);
    // Add log console to the log console VBox
    gtk_box_pack_start(GTK_BOX(log_console_vbox), log_console_treeview, TRUE, TRUE, 0);
    return log_console_vbox;
}

//Memory+Log
GtkWidget* create_memory_log_hbox() {
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

//Process Configuration
// Button click handler to set arrival_time from the entry
void on_set_arrival_time_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *entry = GTK_WIDGET(user_data);
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (text && *text != '\0') {
        char *endptr;
        double value = g_strtod(text, &endptr);
        if (endptr != text && *endptr == '\0') {
            arrival_time = value;
            g_print("Arrival time set to: %.2f\n", arrival_time);
        } else {
            g_print("Invalid input for arrival time.\n");
        }
    }
}

// Function to create the process config HBox with label, entry, and button
GtkWidget* create_process_config_hbox() {
        GtkWidget *process_config_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(process_config_hbox), 5);
    
        // Arrival Time Label 
        GtkWidget *process_arrival_time_label = gtk_label_new("Arrival Time:");
        gtk_widget_set_valign(process_arrival_time_label, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(process_config_hbox), process_arrival_time_label, FALSE, FALSE, 0);
    
        // Entry Field
        GtkWidget *arrival_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(arrival_entry), "Please enter the arrival time");
        gtk_widget_set_size_request(arrival_entry, 230, 35); // Consistent height
        gtk_widget_set_valign(arrival_entry, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(process_config_hbox), arrival_entry, FALSE, FALSE, 0);
    
        // Set Button 
        GtkWidget *set_button = gtk_button_new_with_label("Set");
        gtk_widget_set_size_request(set_button, 80, 35); 
        gtk_widget_set_valign(set_button, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(process_config_hbox), set_button, FALSE, FALSE, 0);
    
        g_signal_connect(G_OBJECT(set_button), "clicked", G_CALLBACK(on_set_arrival_time_clicked), arrival_entry);
    
        return process_config_hbox;
    }

//Process Creation
GtkWidget* create_process_creation(){
    GtkWidget* process_creation_hbox= gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(process_creation_hbox), 10);
    //Add Process Button
    GtkWidget *add_button = gtk_button_new_with_label("Add Process");
    gtk_widget_set_size_request(add_button, 120, 40); 
    gtk_widget_set_valign(add_button, GTK_ALIGN_CENTER); 
    gtk_box_pack_start(GTK_BOX(process_creation_hbox), add_button, FALSE, FALSE, 0);

    // Process Configuration
    GtkWidget *process_config_hbox = create_process_config_hbox();
    gtk_widget_set_valign(process_config_hbox, GTK_ALIGN_CENTER); 
    gtk_box_pack_end(GTK_BOX(process_creation_hbox), process_config_hbox, TRUE, TRUE, 0); 
    gtk_widget_set_margin_start(process_creation_hbox, 20); 
    gtk_widget_set_margin_end(process_creation_hbox, 20); 

    return process_creation_hbox;
}
//Changing current instr (for backend)
void append_instruction_log(const gchar *instruction, const gchar *pid, const gchar *reaction) {
    // If there's a valid current row, clean it up before updating
    if (gtk_list_store_iter_is_valid(log_store, &current_iter_instruction_log)) {
        gchar *old_instr = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(log_store), &current_iter_instruction_log, 0, &old_instr, -1); // Get current instruction

        if (old_instr) {
            gchar *cleaned = g_strdup(old_instr);  // full copy
            gchar *original = cleaned;             // backup original pointer for safe free

            // Skip "-> " if it exists
            if (g_str_has_prefix(cleaned, "->"))
                cleaned += 3;

            // Remove " (Current)" if it exists
            gchar *pos = g_strrstr(cleaned, " (Current)");
            if (pos) *pos = '\0';

            gtk_list_store_set(log_store, &current_iter_instruction_log, 0, cleaned, -1);

            g_free(original);  // safe free
            g_free(old_instr); // now we can safely free GTK's return
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
    g_free(marked_instruction);
}

//Execution HBox
GtkWidget* create_execution_hbox(){
    GtkWidget *execution_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(execution_hbox), 10);
    //Buttons
    GtkWidget *steps_button = gtk_button_new_with_label("Step-By-Step Execution");
    gtk_box_pack_start(GTK_BOX(execution_hbox), steps_button, TRUE, TRUE, 0);
    GtkWidget *auto_button = gtk_button_new_with_label("Auto Execution");
    gtk_box_pack_start(GTK_BOX(execution_hbox), auto_button, TRUE, TRUE, 0);
    
    return execution_hbox;
}
//css
void load_styles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    
    if (!gtk_css_provider_load_from_path(provider, "style.css", NULL)) {
        g_warning("Failed to load CSS file");
    }
        gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
}

// Function to activate the GTK application window
static void activate(GtkApplication *app, gpointer user_data) {

    // Load CSS styles
    load_styles();

    // Create main window
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Scheduler Simulation");
    gtk_window_set_default_size(GTK_WINDOW(window), 1500, 950);
    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);

    //Main Container vbox
    GtkWidget* main_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(main_container), 10);
    gtk_container_add(GTK_CONTAINER(scrolled_window), main_container);
    gtk_widget_set_margin_start(main_container, 20);
    gtk_widget_set_margin_end(main_container, 20);

    //Main Dashboard
    GtkWidget* dashboard_frame=create_labeled_frame("<span font='12'><b>Main Dashboard</b></span>");
    gtk_box_pack_start(GTK_BOX(main_container), dashboard_frame, FALSE, FALSE, 0);
    GtkWidget* dashboard_vbox= create_dashboard(); 
    gtk_container_add(GTK_CONTAINER(dashboard_frame), dashboard_vbox);

    // Scheduler Panel
    GtkWidget* scheduler_frame = create_labeled_frame("<span font='12'><b>Scheduler Control Panel</b></span>");
    gtk_box_pack_start(GTK_BOX(main_container), scheduler_frame, FALSE, FALSE, 0);  
    GtkWidget* scheduler_vbox = create_scheduler_vbox();
    gtk_container_add(GTK_CONTAINER(scheduler_frame), scheduler_vbox);

    //Resource Management 
    GtkWidget* resource_frame=create_labeled_frame("<span font='12'><b>Resource Management Panel</b></span>");
    gtk_box_pack_start(GTK_BOX(main_container), resource_frame, FALSE, FALSE, 0);
    GtkWidget* resource_vbox = create_resource_vbox();
    gtk_container_add(GTK_CONTAINER(resource_frame), resource_vbox);

    //Memory Log and Console Panel
    GtkWidget* memory_log_frame = create_labeled_frame("<span font='12'><b>Memory Log &amp; Console Panel</b></span>");
    gtk_box_pack_start(GTK_BOX(main_container), memory_log_frame, FALSE, FALSE, 0);
    GtkWidget *memory_log_hbox = create_memory_log_hbox();
    gtk_container_add(GTK_CONTAINER(memory_log_frame), memory_log_hbox);

    // Process Creation
    GtkWidget* process_creation_frame = create_labeled_frame("<span font='12'><b>Process Creation</b></span>");
    gtk_box_pack_start(GTK_BOX(main_container), process_creation_frame, FALSE, FALSE, 0);
    GtkWidget* process_creation_hbox = create_process_creation();
    gtk_container_add(GTK_CONTAINER(process_creation_frame), process_creation_hbox);

    //Execution Frame
    GtkWidget* execution_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(execution_frame), GTK_SHADOW_IN); 
    gtk_widget_set_size_request(execution_frame, -1, 50);
    gtk_box_pack_start(GTK_BOX(main_container), execution_frame, FALSE, FALSE, 0);
    GtkWidget* execution_hbox = create_execution_hbox();
    gtk_container_add(GTK_CONTAINER(execution_frame), execution_hbox);

    //SAMPLE DATA

    //Process List sample data
    gtk_list_store_insert_with_values(process_list_store, NULL, -1, 
        0, "1", 
        1, "Running", 
        2, "High", 
        3, "0-100", 
        4, "500", 
        -1);
    // Ready Queue sample data
    gtk_list_store_insert_with_values(ready_queue_store, NULL, -1, 
        0, "3", 
        1, "5", 
        -1);
    // Blocked Queue sample data
    gtk_list_store_insert_with_values(blocked_queue_store, NULL, -1, 
        0, "5", 
        1, "7", 
        -1);
    // Running Process sample data
    gtk_list_store_insert_with_values(running_process_store, NULL, -1, 
        0, "7", 
        -1);
    // Mutex Status sample data
    gtk_list_store_insert_with_values(mutex_status_store, NULL, -1, 
            0, "Mutex1", 
            1, "8", 
            2, "9", 
            -1);
        
    // Memory Viewer sample data
    for (int i = 0; i < 60; i++) {
        GtkTreeIter iter;
        gchar *address_str = g_strdup_printf("0x%04X", i); // You can change format if needed
    
        gtk_list_store_append(memory_viewer_store, &iter);
        gtk_list_store_set(memory_viewer_store, &iter,
            0, address_str,  // Set address
            1, "",            // Empty value for now
            -1
        );
    
        g_free(address_str);
    }
    // Log & Console sample data
    
    //instruction log
    append_instruction_log("LOAD R1, A", "P1", "Acquired userquantum_input");
    append_instruction_log("ADD R2, R1", "P2", "Waiting for file");
    append_instruction_log("STORE R2, B", "P3", "Released userquantum_input");

    // Show everything
    g_timeout_add(0, (GSourceFunc)gtk_widget_hide, quantum_button_box);
    gtk_widget_show_all(window);

}

int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("simulation.scheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

