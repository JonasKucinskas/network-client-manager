#include <gtk/gtk.h>
#include "headers/utils.h"
#include "headers/wan.h"

GList *parameter_rows = NULL;
Method *selected_method = NULL;

void alert_popup(const char *header, const char *body)
{
    GtkWidget* msg = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, header);

    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg), body);
    int response = gtk_dialog_run(GTK_DIALOG(msg));
    //printf("response was %d (OK=%d, DELETE_EVENT=%d)\n", response, GTK_RESPONSE_OK, GTK_RESPONSE_DELETE_EVENT);

    gtk_widget_destroy(msg);
}

static void on_submit(GtkButton *button, gpointer user_data) 
{
    GtkWidget *vbox = GTK_WIDGET(user_data);

    for (GList *node = parameter_rows; node != NULL; node = node->next) 
    {
        ParameterWidgets *row = (ParameterWidgets *)node->data;

        //TODO check if values already exist in method parameters
        const gchar *name = gtk_entry_get_text(GTK_ENTRY(row->name)); 
        const gchar *value = gtk_entry_get_text(GTK_ENTRY(row->value)); 


        //empty non null string
        if (name[0] == '\0' || value[0] == '\0')
        {
            continue;
        }

        selected_method->parameters = realloc(
            selected_method->parameters,
            (selected_method->param_count + 1) * sizeof(Parameter)
        );

        if (selected_method->parameters == NULL) 
        {
            perror("Failed to allocate memory for parameters");
            return;
        }

        Parameter *param = &selected_method->parameters[selected_method->param_count];

        param->name = strdup(name); 
        param->value = strdup(value);
        selected_method->param_count++;
    }

    //if user submits params and then decides to add more and submits again,
    //this will prevent param dublication.
    g_list_free_full(parameter_rows, g_free);
    parameter_rows = NULL;

    //TODO free selected method on dialog close.
}

static void on_method_selected(GtkComboBox *combo, gpointer user_data) 
{
    //in case user changed selected method in combo box.
    int selected_method_index = gtk_combo_box_get_active(combo);
    selected_method = &methods[selected_method_index];
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

    gtk_widget_show_all(vbox);
}

void show_parameter_dialog(int method_index) 
{
    selected_method = &methods[method_index];
    GtkWidget *dialog, *vbox, *hbox_combo_button, *combo_box, *label, *add_button, *submit_button;

    dialog = gtk_dialog_new_with_buttons ("Select API Method",
                                         NULL,
                                         GTK_DIALOG_MODAL,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_OK", GTK_RESPONSE_ACCEPT,
                                         NULL);

    //g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), vbox);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);

    label = gtk_label_new("Select an API method:");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);

    //combo box and "Add parameter" button in the same row.
    hbox_combo_button = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    combo_box = gtk_combo_box_text_new();
    
    for (int i = 0; i < method_count; i++) 
    {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_box), methods[i].name);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), method_index);
    gtk_box_pack_start(GTK_BOX(hbox_combo_button), combo_box, TRUE, TRUE, 5);

    g_signal_connect(combo_box, "changed", G_CALLBACK(on_method_selected), dialog);

    add_button = gtk_button_new_with_label("Add Parameter");
    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_parameter), vbox);
    
    gtk_box_pack_start(GTK_BOX(hbox_combo_button), add_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_combo_button, FALSE, FALSE, 5);

    submit_button = gtk_button_new_with_label("Submit");
    g_signal_connect(submit_button, "clicked", G_CALLBACK(on_submit), vbox);
    gtk_box_pack_start(GTK_BOX(vbox), submit_button, FALSE, FALSE, 5);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) 
    {

    }

    gtk_widget_destroy(dialog);
}