#include <gtk/gtk.h>
#include <stdbool.h>

#define BUFSIZE 36
#define CMD_LENGTH_DC_CLIENT 47
#define CMD_LENGTH_STATIONS_GET 43

GtkWidget *client_grid;
int client_count = 0;
gchar **macs = NULL;
GtkTextBuffer **buffers = NULL;
GtkWidget **text_views = NULL;
GtkWidget **dc_buttons = NULL;

static void dc_client(GtkButton *button, gchar *mac) 
{
  char cmd[CMD_LENGTH_DC_CLIENT] = "sudo iw dev wlan0 station del ";
  strcat(cmd, mac);
  
  FILE *fp = popen(cmd, "r");
  
  pclose(fp);
}

static void set_client_macs() 
{
  char *cmd = "iw dev wlan0 station dump | grep 'Station' | awk '{print $2}'";
  FILE *fp = popen(cmd, "r");
  
  size_t outlen = 0;
  char *out = NULL;
  int count = 0;

  while (getline(&out, &outlen, fp) != -1) 
  {
    out[strcspn(out, "\n")] = '\0';

    macs = g_realloc(macs, sizeof(gchar *) * (count + 2));
    macs[count] = g_strdup(out);
    count++;
  }
  macs[count] = NULL;

  free(out);
  pclose(fp);
}

static void remove_child(GtkWidget *widget, gpointer data) 
{
  GtkGrid *client_grid = GTK_GRID(data);
  gtk_container_remove(GTK_CONTAINER(client_grid), widget);
}

static void clear_grid(GtkGrid *client_grid) 
{
  gtk_container_foreach(GTK_CONTAINER(client_grid), remove_child, client_grid);
}

static void create_widgets(int i) 
{
  buffers[i] = gtk_text_buffer_new(NULL);
  text_views[i] = gtk_text_view_new_with_buffer(buffers[i]);
  
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_views[i]), FALSE);
  gtk_grid_attach(GTK_GRID(client_grid), text_views[i], i, 0, 1, 1);
  
  dc_buttons[i] = gtk_button_new_with_label("Disconnect");
  g_signal_connect(dc_buttons[i], "clicked", G_CALLBACK(dc_client), macs[i]);
  gtk_grid_attach(GTK_GRID(client_grid), dc_buttons[i], i, 1, 1, 1);
  gtk_widget_show_all(client_grid);
}

static void update_buffer(int i) 
{
  char cmd[] = "iw dev wlan0 station get ";
  char cmd_full[CMD_LENGTH_STATIONS_GET] = {0};
  
  snprintf(cmd_full, sizeof(cmd_full), "%s%s", cmd, macs[i]);
  
  GtkTextIter end;
  GtkTextIter start;
  
  gtk_text_buffer_get_start_iter(buffers[i], &start);
  gtk_text_buffer_get_end_iter(buffers[i], &end);
  gtk_text_buffer_delete(buffers[i], &start, &end);
  
  char buf[BUFSIZE] = {0};
  FILE *fp;
  
  fp = popen(cmd_full, "r");
  
  while (fgets(buf, BUFSIZE, fp) != NULL) 
  {
    gtk_text_buffer_insert(buffers[i], &end, buf, -1);
  }

  fclose(fp);
}

static void update_gui() 
{
  for (int i = 0; i < client_count; i++) 
  {
    update_buffer(i);
  }
}

gboolean set_client_count(gpointer user_data) 
{
  char *cmd = "iw dev wlan0 station dump | grep 'Station' | wc -l";
  
  FILE *fp = popen(cmd, "r");
  int new_client_count;

  fscanf(fp, "%d", &new_client_count);
  pclose(fp);
  
  if (client_count != new_client_count)
  {
    int old_client_count = client_count;
    client_count = new_client_count;
    
    set_client_macs();

    if (buffers != NULL) 
    {
      for (int i = 0; i < old_client_count; i++)
      {
        g_object_unref(buffers[i]);
        gtk_widget_destroy(text_views[i]);
        gtk_widget_destroy(dc_buttons[i]);
      }
      g_free(buffers);
      g_free(text_views);
      g_free(dc_buttons);
      clear_grid(GTK_GRID(client_grid));
    }

    buffers = g_new0(GtkTextBuffer*, client_count);
    text_views = g_new0(GtkWidget*, client_count);
    dc_buttons = g_new0(GtkWidget*, client_count);
    
    for (int i = 0; i < client_count; i++) 
    {
      create_widgets(i);
    }
  }
  
  update_gui();

  return TRUE;
}