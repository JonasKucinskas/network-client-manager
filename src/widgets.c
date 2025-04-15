#include <gtk/gtk.h>
#include "headers/utils.h"
#include "headers/wan.h"
#include "headers/api.h"

GList *parameter_rows = NULL;
GtkWidget *method_list_box = NULL;
GList *parameter_widgets = NULL;

GtkWidget *username_text_entry = NULL;
GtkWidget *password_text_entry = NULL;
GtkWidget *base_url_text_entry = NULL;

int selected_method_index = -1;

void alert_popup(const char *header, const char *body)
{
    GtkWidget* msg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, header);

    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg), body);
    int response = gtk_dialog_run(GTK_DIALOG(msg));
    //printf("response was %d (OK=%d, DELETE_EVENT=%d)\n", response, GTK_RESPONSE_OK, GTK_RESPONSE_DELETE_EVENT);

    gtk_widget_destroy(msg);
}

static void draw_initial_parameters(GtkWidget *vbox)
{
    size_t param_count = method_container->methods[selected_method_index].param_count;

    for (size_t i = 0; i < param_count; i++)
    {
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); 

        const gchar *param_name = method_container->methods[selected_method_index].parameters[i].name;
        const gchar *param_value = method_container->methods[selected_method_index].parameters[i].value;

        GtkEntryBuffer* param_name_buffer = gtk_entry_buffer_new(param_name, strlen(param_name));
        GtkEntryBuffer* param_value_buffer = gtk_entry_buffer_new(param_value, strlen(param_value));

        GtkWidget *name_label = gtk_label_new("Name:");
        GtkWidget *name_entry = gtk_entry_new_with_buffer(param_name_buffer);
        GtkWidget *value_label = gtk_label_new("Value:");
        GtkWidget *value_entry = gtk_entry_new_with_buffer(param_value_buffer);

        ParameterWidgets *new_row = g_new(ParameterWidgets, 1);
        new_row->name = name_entry;
        new_row->value = value_entry;
        parameter_rows = g_list_append(parameter_rows, new_row);

        gtk_box_pack_start(GTK_BOX(hbox), name_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), name_entry, FALSE, FALSE, 5);

        gtk_box_pack_start(GTK_BOX(hbox), value_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), value_entry, FALSE, FALSE, 5);
        
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

        parameter_widgets = g_list_append(parameter_widgets, hbox);
    }
}

static void on_parameters_submit(GtkButton *button, gpointer user_data) 
{
    GtkWidget *vbox = GTK_WIDGET(user_data);
    Method *selected_method = &method_container->methods[selected_method_index];

    for (GList *node = parameter_rows; node != NULL;) 
    {
        ParameterWidgets *row = (ParameterWidgets *)node->data;

        const gchar *param_name = gtk_entry_get_text(GTK_ENTRY(row->name)); 
        const gchar *param_value = gtk_entry_get_text(GTK_ENTRY(row->value)); 

        gint i = g_list_index(parameter_rows, node->data);

        //remove param
        if (param_name[0] == '\0' || param_value[0] == '\0')//empty, non null string
        {
            //empty param's index in ui is less than param count, meaning user deleted that param.
            if (i < selected_method->param_count)
            {
                //free deleted param
                free(selected_method->parameters[i].name);
                free(selected_method->parameters[i].value);

                //swap deleted param with last parameter, then realloc.
                selected_method->parameters[i] = selected_method->parameters[selected_method->param_count - 1];
                selected_method->parameters = realloc(selected_method->parameters, (selected_method->param_count - 1) * sizeof(Parameter));
                selected_method->param_count -= 1;
            }

            //remove text fields in ui:
            GList* i_th_param_widget = g_list_nth(parameter_widgets, i);
            GtkWidget *hbox = (GtkWidget *)i_th_param_widget->data;
            gtk_widget_destroy(hbox);
            parameter_widgets = g_list_delete_link (parameter_widgets, i_th_param_widget);

            //remove param row.            
            GList *next_node = node->next;  
            parameter_rows = g_list_delete_link(parameter_rows, node);
            
            node = next_node;
            continue;
        }

        //edit param
        if (i + 1 <= selected_method->param_count)
        {
            gboolean name_edited = g_strcmp0(param_name, selected_method->parameters[i].name) != 0;
            gboolean value_edited = g_strcmp0(param_value, selected_method->parameters[i].value) != 0;

            if (name_edited || value_edited)
            {
                selected_method->parameters[i].name = strdup(param_name);
                selected_method->parameters[i].value = strdup(param_value);
                
                node = node->next;
                continue;
            }
        }

        //index bigger than param count, add parameter.
        if (i + 1 > selected_method->param_count)
        {
            //add param:
            selected_method->parameters = realloc(selected_method->parameters, (selected_method->param_count + 1) * sizeof(Parameter));

            if (selected_method->parameters == NULL) 
            {
                g_print("Failed to allocate memory for new parameter");
                return;
            }

            selected_method->parameters[i].name = strdup(param_name);
            selected_method->parameters[i].value = strdup(param_value);
            selected_method->param_count++;

            node = node->next;
            continue;
        }
        node = node->next;
    }

    if (selected_method->param_count > 0)
    {
        gboolean wrote_json = write_params_json(selected_method); 

        if (!wrote_json)
        {
            alert_popup("Error", "Failed to update parameters.");
        }
        else
        {
            alert_popup("Success", "Successfuly updated parameters.");
        }
    }
}

static void clear_param_data(gpointer user_data)
{
    //if user submits params and then decides to add more and submits again,
    //this will prevent param dublication.
    g_list_free_full(parameter_rows, g_free);
    parameter_rows = NULL;
    
    //free list, but keep widgets loaded (they get deleted when window gets closed):
    g_list_free(parameter_widgets);
    parameter_widgets = NULL;
}

static void on_method_selected(GtkComboBox *combo, gpointer user_data) 
{
    for (GList *node = parameter_widgets; node != NULL; ) 
    {
        GtkWidget *hbox = (GtkWidget *)node->data;
        gtk_widget_destroy(hbox); 
        GList *next_node = node->next;
        parameter_widgets = g_list_delete_link(parameter_widgets, node);
        node = next_node; 
    }
    clear_param_data(NULL);

    //in case user changed selected method in combo box.
    selected_method_index = gtk_combo_box_get_active(combo);
    if (selected_method_index == -1) return;

    GtkWidget *vbox = GTK_WIDGET(user_data);
    draw_initial_parameters(vbox);
    gtk_widget_show_all(vbox);
}

static void on_remove_method(GtkButton *button, gpointer user_data) 
{
    char *selected_method_name = (char*)(user_data);

    GtkWidget *dialog = gtk_dialog_new_with_buttons ("Delete Method",
                                        NULL,
                                        GTK_DIALOG_MODAL,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_OK", GTK_RESPONSE_ACCEPT,
                                        NULL);

    GtkWidget *label = gtk_label_new("Are you sure you want to delete this method?");
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label);

    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response != GTK_RESPONSE_ACCEPT)
    {
        return;
    }
     
    //if user deletes a method, index'es change in memory, 
    //so i need to find the index by name first
    int method_index = find_method_index(selected_method_name);
    gboolean removed = remove_method_from_memory(method_index);

    if (removed == FALSE)
    {
        g_print("failed to remove method from memory");
        return;
    }
    
    //update ui
    GtkListBoxRow* row_to_delete = gtk_list_box_get_row_at_index(GTK_LIST_BOX(method_list_box), method_index);
    gtk_container_remove(GTK_CONTAINER(method_list_box), GTK_WIDGET(row_to_delete));
    
    remove_row_from_model(selected_method_name);

    remove_method_from_json(method_index);
    
    gtk_widget_destroy(dialog);
}

gboolean filter_func(GtkListBoxRow *row, gpointer user_data) 
{
    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(user_data));
    GtkWidget *label = g_object_get_data(G_OBJECT(row), "label");

    const gchar *row_text = gtk_label_get_text(GTK_LABEL(label));

    gchar *row_lower = g_utf8_strdown(row_text, -1);
    gchar *search_lower = g_utf8_strdown(search_text, -1);

    gboolean visible = g_strrstr(row_lower, search_lower) != NULL;

    g_free(row_lower);
    g_free(search_lower);

    return visible;
}

static void on_search_changed(GtkEntry *entry, gpointer user_data) 
{
    gtk_list_box_invalidate_filter(GTK_LIST_BOX(method_list_box));
}

static void draw_method_list_box_row(const char *name)
{
    GtkWidget *box_row = gtk_list_box_row_new();
    GtkWidget *container_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label = gtk_label_new(name);

    gtk_widget_set_halign(label, GTK_ALIGN_START);
    
    GtkWidget *delete_button = gtk_button_new_with_label("Delete");
    g_signal_connect(delete_button, "clicked", G_CALLBACK(on_remove_method), g_strdup(name));

    gtk_container_add(GTK_CONTAINER(container_box), label);
    gtk_box_pack_end(GTK_BOX(container_box), delete_button, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(box_row), container_box);

    g_object_set_data(G_OBJECT(box_row), "label", label);

    gtk_list_box_insert(GTK_LIST_BOX(method_list_box), box_row, method_container->method_count);
    gtk_widget_show_all(box_row); 
}

static void on_add_method(gpointer user_data)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons ("Add method",
                                         NULL,
                                         GTK_DIALOG_MODAL,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_OK", GTK_RESPONSE_ACCEPT,
                                         NULL);

    GtkWidget *text_entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), text_entry);

    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response != GTK_RESPONSE_ACCEPT)
    {
        return;
    }

    const char *method_name = gtk_entry_get_text(GTK_ENTRY(text_entry));
    add_method_to_memory(method_name);
    
    draw_method_list_box_row(method_name);        
    add_row_to_model(method_name);
    write_method_to_json(method_name);


    gtk_widget_destroy(dialog);
}

static void on_add_parameter(GtkButton *button, gpointer user_data) 
{
    GtkWidget *vbox = GTK_WIDGET(user_data);
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); 


    GtkWidget *name_label = gtk_label_new("Name:");
    GtkWidget *name_entry = gtk_entry_new();
    GtkWidget *value_label = gtk_label_new("Value:");
    GtkWidget *value_entry = gtk_entry_new();

    gtk_box_pack_start(GTK_BOX(hbox), name_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), name_entry, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(hbox), value_label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), value_entry, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    ParameterWidgets *new_row = g_new(ParameterWidgets, 1);
    new_row->name = name_entry;
    new_row->value = value_entry;

    parameter_rows = g_list_append(parameter_rows, new_row);
    parameter_widgets = g_list_append(parameter_widgets, hbox);

    gtk_widget_show_all(vbox);
}

static GtkWidget* draw_param_dialog_content(int method_index)
{
    selected_method_index = method_index;

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *label = gtk_label_new("Select an API method:");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);

    //combo box and "Add parameter" button in the same row.
    GtkWidget *hbox_combo_button = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *combo_box = gtk_combo_box_text_new();
    
    for (int i = 0; i < method_container->method_count; i++) 
    {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), method_container->methods[i].name);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), method_index);
    gtk_box_pack_start(GTK_BOX(hbox_combo_button), combo_box, TRUE, TRUE, 5);

    g_signal_connect(combo_box, "changed", G_CALLBACK(on_method_selected), vbox);

    GtkWidget *add_button = gtk_button_new_with_label("Add Parameter");
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_parameter), vbox);
    
    gtk_box_pack_start(GTK_BOX(hbox_combo_button), add_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_combo_button, FALSE, FALSE, 5);

    GtkWidget *submit_button = gtk_button_new_with_label("Submit");
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_parameters_submit), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), submit_button, FALSE, FALSE, 5);

    draw_initial_parameters(vbox);
    return vbox;
}

static GtkWidget* draw_methods_page_content()
{
    //place search box and "Add" button in header hbox
    //place method_page in scrollable window
    //place header hbox and scrollable window into vbox
    GtkWidget *page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);


    GtkWidget *header_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    GtkWidget *search_bar = gtk_search_entry_new();
    g_signal_connect(search_bar, "changed", G_CALLBACK(on_search_changed), NULL);
    gtk_box_pack_start(GTK_BOX(header_hbox), search_bar, TRUE, TRUE, 5);

    GtkWidget *add_method_button = gtk_button_new_with_label("Add");
    g_signal_connect(add_method_button, "clicked", G_CALLBACK(on_add_method), NULL);
    gtk_box_pack_end(GTK_BOX(header_hbox), add_method_button, FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(page_vbox), header_hbox, FALSE, FALSE, 5);
    //

    GtkWidget *method_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    method_list_box = gtk_list_box_new();
    gtk_widget_set_vexpand(method_list_box, TRUE);
    gtk_widget_set_hexpand(method_list_box, TRUE);

    gtk_list_box_set_filter_func(GTK_LIST_BOX(method_list_box), filter_func, search_bar, NULL);

    for (int i = 0; i < method_container->method_count; i++)
    {
        Method *method = &method_container->methods[i];
        draw_method_list_box_row(method->name);
    }
    
    gtk_box_pack_start(GTK_BOX(method_vbox), method_list_box, TRUE, TRUE, 5);

    
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), method_vbox);

    gtk_box_pack_start(GTK_BOX(page_vbox), scrolled_window, TRUE, TRUE, 5);


    return page_vbox;
}

static void on_user_details_submit(GtkButton *button, gpointer user_data)
{
    GtkBox *page_vbox = GTK_BOX(user_data);

    const gchar *username_new = gtk_entry_get_text(GTK_ENTRY(username_text_entry));
    const gchar *password_new = gtk_entry_get_text(GTK_ENTRY(password_text_entry));
    const gchar *base_url_new = gtk_entry_get_text(GTK_ENTRY(base_url_text_entry));

    username = g_strdup(username_new);
    password = g_strdup(password_new);
    base_url = g_strdup(base_url_new);

    //write details to json
    gboolean wrote_json = write_user_details_to_json();

    if (!wrote_json)
    {
        alert_popup("Error", "Failed to write to config file.");
    }
    else
    {
        alert_popup("Success", "Successfuly updated user details.");
    }
}

static GtkWidget* draw_user_page_content()
{
    GtkWidget *page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);


    GtkWidget *username_label = gtk_label_new ("Username:");

    GtkEntryBuffer *username_buffer = gtk_entry_buffer_new(username, strlen(username));
    username_text_entry = gtk_entry_new_with_buffer(username_buffer);

    gtk_box_pack_start(GTK_BOX(page_vbox), username_label, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(page_vbox), username_text_entry, FALSE, FALSE, 0); 


    GtkWidget *password_label = gtk_label_new ("Password:");

    GtkEntryBuffer *password_buffer = gtk_entry_buffer_new(password, strlen(password));
    password_text_entry = gtk_entry_new_with_buffer(password_buffer);

    gtk_box_pack_start(GTK_BOX(page_vbox), password_label, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(page_vbox), password_text_entry, FALSE, FALSE, 0); 


    GtkWidget *base_url_label = gtk_label_new ("Default url:");

    GtkEntryBuffer *base_url_buffer = gtk_entry_buffer_new(base_url, strlen(base_url));
    base_url_text_entry = gtk_entry_new_with_buffer(base_url_buffer);

    gtk_box_pack_start(GTK_BOX(page_vbox), base_url_label, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(page_vbox), base_url_text_entry, FALSE, FALSE, 0); 


    GtkWidget *submit_button = gtk_button_new_with_label("Submit");
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_user_details_submit), page_vbox);
    gtk_box_pack_start(GTK_BOX(page_vbox), submit_button, FALSE, FALSE, 5);


    return page_vbox;
}

void show_parameter_dialog(int method_index) 
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons ("Select API Method",
                                         NULL,
                                         GTK_DIALOG_MODAL,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_OK", GTK_RESPONSE_ACCEPT,
                                         NULL);

    //g_signal_connect(dialog, "response", G_CALLBACK(clear_param_data), NULL);

    GtkWidget* vbox = draw_param_dialog_content(method_index);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);

    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    clear_param_data(NULL);

    gtk_widget_destroy(dialog);
}

void open_menu_window()
{
    GtkWidget *stack = gtk_stack_new();

    GtkWidget *param_page = draw_param_dialog_content(0);
    GtkWidget *method_page = draw_methods_page_content();
    GtkWidget *user_page = draw_user_page_content();

    gtk_stack_add_titled(GTK_STACK(stack), method_page, "method_page", "Methods");
    gtk_stack_add_titled(GTK_STACK(stack), param_page, "param_page", "Parameters");
    gtk_stack_add_titled(GTK_STACK(stack), user_page, "user_page", "User");

    GtkWidget *stack_sidebar = gtk_stack_sidebar_new();
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(stack_sidebar), GTK_STACK(stack));

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Settings",
                                                    NULL,
                                                    GTK_DIALOG_MODAL,
                                                    "_Cancel", GTK_RESPONSE_CANCEL,
                                                    "_OK", GTK_RESPONSE_ACCEPT,
                                                    NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    gtk_box_pack_start(GTK_BOX(box), stack_sidebar, FALSE, FALSE, 0); 
    gtk_box_pack_start(GTK_BOX(box), stack, TRUE, TRUE, 0); 

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), box);

    gtk_widget_show_all(dialog);

    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    clear_param_data(NULL);
    gtk_widget_destroy(dialog);
}