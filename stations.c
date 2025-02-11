#include <gtk/gtk.h>
#include <stdbool.h>
#define BUFSIZE 64

static gboolean set_input(GtkTextBuffer *buffer)
{
  char *cmd = "iw dev wlan0 station dump";
  char buf[BUFSIZE] = {0};
  FILE *fp;

  if ((fp = popen(cmd, "r")) == NULL)
  {
    g_print("error opening pipe");
    return FALSE;
  } 

  GtkTextIter end;
  GtkTextIter start;

  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);

  gtk_text_buffer_delete(buffer, &start, &end);

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
  GtkWidget *button;

  window = gtk_application_window_new (app);
  gtk_window_set_title(GTK_WINDOW (window), "Clients");
  gtk_window_maximize(GTK_WINDOW (window));
  
  grid = gtk_grid_new();
  gtk_container_add (GTK_CONTAINER (window), grid);

  //text box set
  GtkTextBuffer *buffer = gtk_text_buffer_new(NULL);
  
  //refresh client info
  g_timeout_add(1000, (GSourceFunc)set_input, buffer);

  text_view = gtk_text_view_new_with_buffer(buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);

  gtk_grid_attach(GTK_GRID(grid), text_view, 0, 0, 1, 1);

  //gtk_container_add (GTK_CONTAINER (grid), text_view);
  //button set;

  button = gtk_button_new_with_label ("off");
  gtk_grid_attach(GTK_GRID(grid), button, 0, 1, 1, 1);




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
