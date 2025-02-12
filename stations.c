#include <gtk/gtk.h>
#include <stdbool.h>
#define BUFSIZE 64

static gchar* get_client_mac(GtkTextBuffer *buffer)
{
  GtkTextIter start_iter;
  GtkTextIter end_iter;

  //mac address is between 8th and 25th chars in buffer at the first line.
  gtk_text_buffer_get_iter_at_line_offset(buffer, &start_iter, 0, 8);
  gtk_text_buffer_get_iter_at_line_offset(buffer, &end_iter, 0, 25);
  
  gchar *mac = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
  return mac;
}

static void dc_client(GtkButton *button, GtkTextBuffer *buffer)
{ 
  const gchar *mac = get_client_mac(buffer);

  char cmd[47] = "sudo iw dev wlan0 station del ";
  strcat(cmd, mac);

  FILE *fp = popen(cmd, "r");
  fclose(fp);
}

//iw dev wlan0 station del ca:1c:72:9e:79:d2
static gboolean set_input(GtkTextBuffer *buffer)
{
  char *cmd = "iw dev wlan0 station dump";
  char buf[BUFSIZE] = {0};
  FILE* fp;


  GtkTextIter end;
  GtkTextIter start;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);
  gtk_text_buffer_delete(buffer, &start, &end);

  fp = popen(cmd, "r");
  if (fp == NULL)
  {
    //print error;
  }

  while (fgets(buf, BUFSIZE, fp) != NULL)
  {
    gtk_text_buffer_insert(buffer, &end, buf, -1);
  }

  fclose(fp);
  return TRUE;
}

static void activate (GtkApplication *app, gpointer user_data)
{
  GtkWidget *window;
  GtkWidget *text_view;
  GtkWidget *grid;
  GtkTextBuffer* buffer;

  window = gtk_application_window_new (app);
  gtk_window_set_title(GTK_WINDOW (window), "Clients");
  gtk_window_maximize(GTK_WINDOW (window));
  
  grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER (window), grid);

  buffer = gtk_text_buffer_new(NULL);

  //refresh client info
  g_timeout_add(1000, (GSourceFunc)set_input, buffer);

  text_view = gtk_text_view_new_with_buffer(buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

  gtk_grid_attach(GTK_GRID(grid), text_view, 0, 0, 2, 1);

  //gtk_container_add(GTK_CONTAINER (grid), text_view);
  
  //set buttons
  //column 0, row 1, span 2 horizontaly, 1 verticaly
  GtkWidget *dc_button = gtk_button_new_with_label("Disconnect");
  g_signal_connect(dc_button, "clicked", G_CALLBACK (dc_client), buffer);
  gtk_grid_attach(GTK_GRID(grid), dc_button, 0, 1, 2, 1);

  gtk_widget_show_all (window);
}

int main (int argc, char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}