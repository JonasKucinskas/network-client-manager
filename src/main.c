#include <gtk/gtk.h>
#include "stations.h" 
#include "wan.h" 

#define UPDATE_RATE 1000

gint client_page;
gint wan_page;

static guint client_update_id = 0;

static void on_page_switched(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
{
    if (page_num == client_page) 
    {
        //add the timeout if its not already active
        if (client_update_id == 0) {  
            client_update_id = g_timeout_add(UPDATE_RATE, (GSourceFunc)set_client_count, NULL);
        }
    } 
    else if (page_num == wan_page) 
    {
        //remove the timeout
        if (client_update_id != 0) {
            g_source_remove(client_update_id);
            client_update_id = 0;
        }

        draw_expander();
    }
}

void activate(GtkApplication *app, gpointer user_data) 
{
  GtkWidget *window;
  
  window = gtk_application_window_new(app);
  
  gtk_window_set_title(GTK_WINDOW(window), "Network");
  gtk_window_maximize(GTK_WINDOW(window));
  
  client_grid = gtk_grid_new();
  wan_grid = gtk_grid_new();

  GtkWidget *notebook = gtk_notebook_new();
  client_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), GTK_WIDGET(client_grid), gtk_label_new("Clients"));
  wan_page = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), GTK_WIDGET(wan_grid), gtk_label_new("Wan"));

  g_signal_connect(notebook, "switch-page", G_CALLBACK(on_page_switched), NULL);

  gtk_container_add(GTK_CONTAINER(window), notebook);
  
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
