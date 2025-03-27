#include <gtk/gtk.h>
#include "headers/utils.h"
#include "headers/wan.h"
#include "headers/api.h"

GList *parameter_rows = NULL;
GList *parameter_widgets = NULL;

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

static void on_submit(GtkButton *button, gpointer user_data) 
{
    GtkWidget *vbox = GTK_WIDGET(user_data);
    Method *selected_method = &method_container->methods[selected_method_index];

    for (GList *node = parameter_rows; node != NULL; node = node->next) 
    {
        ParameterWidgets *row = (ParameterWidgets *)node->data;

        const gchar *param_name = gtk_entry_get_text(GTK_ENTRY(row->name)); 
        const gchar *param_value = gtk_entry_get_text(GTK_ENTRY(row->value)); 

        gint i = g_list_index(parameter_rows, node->data);

        //remove param
        if (param_name[0] == '\0' || param_value[0] == '\0')//empty, non null string
        {
            //empty param's index in ui is less than param count, meaning user deleted that param.
            if (i <= selected_method->param_count - 1)
            {
                free(selected_method->parameters[i].name);
                free(selected_method->parameters[i].value);

                //free deleted param and swap it with last parameter, then realloc.
                selected_method->parameters[i] = selected_method->parameters[selected_method->param_count - 1];
                selected_method->parameters = realloc(selected_method->parameters, (selected_method->param_count - 1) * sizeof(Parameter));
                selected_method->param_count -= 1;
            }

            //remove text fields in ui:
            GList* i_th_param_widget = g_list_nth(parameter_widgets, i);
            GtkWidget *hbox = (GtkWidget *)i_th_param_widget->data;
            gtk_widget_destroy(hbox);
            parameter_widgets = g_list_delete_link (parameter_widgets, i_th_param_widget);
            
            //remove text values from memory
            parameter_rows = g_list_delete_link(parameter_rows, node);

            continue;
        }

        //edit param

        if (i <= selected_method->param_count - 1)
        {
            gboolean name_edited = g_strcmp0(param_name, selected_method->parameters[i].name) != 0;
            gboolean value_edited = g_strcmp0(param_value, selected_method->parameters[i].value) != 0;

            if (name_edited || value_edited)
            {
                selected_method->parameters[i].name = strdup(param_name);
                selected_method->parameters[i].value = strdup(param_value);
                continue;
            }
        }

        //index bigger than param count, add parameters.
        if (i > selected_method->param_count - 1)
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
        }
    }

    write_params_json(selected_method); 
}

static void on_method_selected(GtkComboBox *combo, gpointer user_data) 
{
    //in case user changed selected method in combo box.
    selected_method_index = gtk_combo_box_get_active(combo);
    if (selected_method_index == -1) return;

    GtkWidget *dialog = GTK_WIDGET(user_data);
    gtk_widget_show_all(dialog);
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

void show_parameter_dialog(int method_index) 
{
    selected_method_index = method_index;

    GtkWidget *dialog = gtk_dialog_new_with_buttons ("Select API Method",
                                         NULL,
                                         GTK_DIALOG_MODAL,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_OK", GTK_RESPONSE_ACCEPT,
                                         NULL);

    //g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), vbox);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);

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

    g_signal_connect(combo_box, "changed", G_CALLBACK(on_method_selected), dialog);

    GtkWidget *add_button = gtk_button_new_with_label("Add Parameter");
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_parameter), vbox);
    
    gtk_box_pack_start(GTK_BOX(hbox_combo_button), add_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_combo_button, FALSE, FALSE, 5);

    GtkWidget *submit_button = gtk_button_new_with_label("Submit");
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), submit_button, FALSE, FALSE, 5);


    draw_initial_parameters(vbox);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) 
    {
        //if user submits params and then decides to add more and submits again,
        //this will prevent param dublication.
        g_list_free_full(parameter_rows, g_free);
        parameter_rows = NULL;
    
        //free list, but keep widgets loaded (they get deleted when window gets closed):
        g_list_free(parameter_widgets);
        parameter_widgets = NULL;
    }

    gtk_widget_destroy(dialog);
}