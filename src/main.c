#include <gtk/gtk.h>
#include "headers/stations.h" 
#include "headers/wan.h" 
#include "headers/widgets.h"

#define UPDATE_RATE 1000

gint client_page;
gint wan_page;
gboolean wan_page_drawn = FALSE;

static guint client_update_id = 0;

static void on_page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
{
    if (page_num == client_page) 
    {
        //add the timeout if its not already active
        if (client_update_id == 0) {  
            //call function manualy, so that data is rendered instantly, 
            //before timemout calls the function.
            set_client_count(NULL);
            client_update_id = g_timeout_add(UPDATE_RATE, (GSourceFunc)set_client_count, NULL);
        }
    } 
    else if (page_num == wan_page) 
    {
        //remove the timeout
        if (client_update_id != 0)
        {
            g_source_remove(client_update_id);
            client_update_id = 0;
        }
        
        if (!wan_page_drawn) 
        {
            init_wan_page();  
            wan_page_drawn = TRUE;
        }
    }
}

void activate(GtkApplication *app, gpointer user_data) 
{
    GtkWidget *window;
    
    window = gtk_application_window_new(app);
    
    gtk_window_set_title(GTK_WINDOW(window), "Network");
    gtk_window_maximize(GTK_WINDOW(window));
    
    client_grid = gtk_grid_new();
    wan_view = gtk_tree_view_new ();


    GtkWidget *search_bar = gtk_search_entry_new();
    g_signal_connect(search_bar, "changed", G_CALLBACK(on_search_changed_wan), NULL);

    GtkWidget *wan_page_main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);


    GtkWidget *menu_button = gtk_button_new_with_label("Settings");
    g_signal_connect(menu_button, "clicked", G_CALLBACK(open_settings_window), NULL);

    gtk_box_pack_start(GTK_BOX(header_box), search_bar, TRUE, TRUE, 5);
    gtk_box_pack_end(GTK_BOX(header_box), menu_button, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(wan_page_main_box), header_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(wan_page_main_box), wan_view, FALSE, FALSE, 0);


    GtkWidget *notebook = gtk_notebook_new();
    client_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), GTK_WIDGET(client_grid), gtk_label_new("Clients"));
    wan_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), GTK_WIDGET(wan_page_main_box), gtk_label_new("Wan"));


    g_signal_connect(notebook, "switch-page", G_CALLBACK(on_page_switched), NULL);


    GtkWidget *scroll_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(window), scroll_window);
    gtk_container_add(GTK_CONTAINER(scroll_window), notebook);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) 
{
    GtkApplication *app;
    int status;
    
    app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}