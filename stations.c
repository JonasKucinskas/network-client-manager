#include <gtk/gtk.h>
#include <stdbool.h>
#define BUFSIZE 64
#define LINE_COUNT_PER_CLIENT 19

static int get_client_count(GtkTextBuffer *buffer)
{
  gint line_count = gtk_text_buffer_get_line_count(buffer);

  return line_count / LINE_COUNT_PER_CLIENT;
}

static gchar** get_client_macs(GtkTextBuffer *buffer, int client_count)
{
  GtkTextIter start_iter;
  GtkTextIter end_iter;

  gchar** macs = g_new(gchar*, client_count);
  gint line;

  for (int i = 0; i < client_count; i++)
  {
    line = i * LINE_COUNT_PER_CLIENT;

    //mac address is between 8th and 25th chars in buffer at the first line.
    gtk_text_buffer_get_iter_at_line_offset(buffer, &start_iter, line, 8);
    gtk_text_buffer_get_iter_at_line_offset(buffer, &end_iter, line, 25);
  
    macs[i] = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, FALSE);
  }  

  return macs;
}

static void dc_client(GtkButton *button, gchar *mac)
{ 
  char cmd[47] = "sudo iw dev wlan0 station del ";
  strcat(cmd, mac);

  FILE *fp = popen(cmd, "r");
  fclose(fp);
}

static gboolean set_input(GtkTextBuffer *buffer)
{
  char *cmd = "iw dev wlan0 station dump";
  char buf[BUFSIZE] = {0};
  FILE* fp;

  GtkTextIter end;
  gtk_text_buffer_get_end_iter(buffer, &end);


  fp = popen(cmd, "r");
  while (fgets(buf, BUFSIZE, fp) != NULL)
  {
    gtk_text_buffer_insert(buffer, &end, buf, -1);
  }

  fclose(fp);
  return TRUE;
}

static void set_buffers(GtkTextBuffer** buffers, gchar** macs, int client_count)
{ 
  char cmd[] = "iw dev wlan0 station get ";
  char cmd_full[43] = {0};

  for (int i = 0; i < client_count; i++)
  {
    snprintf(cmd_full, sizeof(cmd_full), "%s%s", cmd, macs[i]);

    char buf[BUFSIZE] = {0};
    FILE* fp;

    buffers[i] = gtk_text_buffer_new(NULL);

    //needed for updates
    GtkTextIter end;
    GtkTextIter start;

    gtk_text_buffer_get_start_iter(buffers[i], &start);
    gtk_text_buffer_get_end_iter(buffers[i], &end);
    gtk_text_buffer_delete(buffers[i], &start, &end);
    //

    fp = popen(cmd_full, "r");

    while (fgets(buf, BUFSIZE, fp) != NULL)
    {
      gtk_text_buffer_insert(buffers[i], &end, buf, -1);
    }

    fclose(fp);
  }
}

static void activate (GtkApplication *app, gpointer user_data)
{
  GtkWidget *window;
  GtkWidget *text_view;
  GtkWidget *grid;
  GtkTextBuffer *buffer;

  window = gtk_application_window_new (app);
  gtk_window_set_title(GTK_WINDOW (window), "Clients");
  gtk_window_maximize(GTK_WINDOW (window));
  
  grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER (window), grid);

  buffer = gtk_text_buffer_new(NULL);
  //initial buffer load
  set_input(buffer);

  int client_count = get_client_count(buffer);
  //g_timeout_add(1000, (GSourceFunc)get_client_count, buffer, &client_count);
  
  //refresh client info
  //g_timeout_add(1000, (GSourceFunc)set_input, buffer);

  GtkTextBuffer **buffers = g_new(GtkTextBuffer*, client_count);
  gchar **macs = get_client_macs(buffer, client_count);

  set_buffers(buffers, macs, client_count);

  for (int i = 0; i < client_count; i++)
  {
    text_view = gtk_text_view_new_with_buffer(buffers[i]);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    
    //2 horizontaly, 1 verticaly
    gtk_grid_attach(GTK_GRID(grid), text_view, 0, i * 2, 2, 1);
    
    
    //set button
    //column 0, row 1, span 2 horizontaly, 1 verticaly
    GtkWidget *dc_button = gtk_button_new_with_label("Disconnect");
    g_signal_connect(dc_button, "clicked", G_CALLBACK (dc_client), macs[i]);
    gtk_grid_attach(GTK_GRID(grid), dc_button, 0, i * 2 + 1, 2, 1);

  }

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