#include <gtk/gtk.h>
#include <stdbool.h>
#define BUFSIZE 64
#define UPDATE_RATE 1000

GtkWidget *grid;
int client_count = 0;
gchar **macs;

static void dc_client(GtkButton *button, gchar *mac)
{ 
  char cmd[47] = "sudo iw dev wlan0 station del ";
  
  strcat(cmd, mac);
  FILE *fp = popen(cmd, "r");
  
  fclose(fp);
}

static void set_client_macs()
{
  char *cmd = "iw dev wlan0 station dump | grep 'Station' | awk '{print $2}'";
  
  FILE *fp = popen(cmd, "r");

  macs = g_new(gchar*, client_count);
  
  size_t outlen = 0;
  char *out = NULL;

  for (int i = 0; i < client_count; i++)
  {
    if (getline(&out, &outlen, fp) == -1) 
    {
      break;
    }
    
    out[strcspn(out, "\n")] = '\0';
    macs[i] = g_strdup(out);
  }

  free(out); 
  fclose(fp);
}

static void set_widgets(GtkTextBuffer *buffer, gchar* mac)
{
  GtkWidget *text_view = gtk_text_view_new_with_buffer(buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    
  //2 horizontaly, 1 verticaly
  gtk_grid_attach(GTK_GRID(grid), text_view, 0, (client_count - 1) * 2, 2, 1);
    
  //set button
  //column 0, row 1, span 2 horizontaly, 1 verticaly
  GtkWidget *dc_button = gtk_button_new_with_label("Disconnect");
  g_signal_connect(dc_button, "clicked", G_CALLBACK (dc_client), mac);
  gtk_grid_attach(GTK_GRID(grid), dc_button, 0, (client_count - 1) * 2 + 1, 2, 1);

  gtk_widget_show(text_view);
  gtk_widget_show(dc_button);
}

static void set_buffer(GtkTextBuffer* buffer, gchar *mac)
{ 
  char cmd[] = "iw dev wlan0 station get ";
  char cmd_full[43] = {0};
  
  snprintf(cmd_full, sizeof(cmd_full), "%s%s", cmd, mac);
    
  GtkTextIter end;
  GtkTextIter start;
    
  gtk_text_buffer_get_start_iter(buffer, &start);
  gtk_text_buffer_get_end_iter(buffer, &end);
    
  //clear old data before writing.
  gtk_text_buffer_delete(buffer, &start, &end);
  //
    
  char buf[BUFSIZE] = {0};
  FILE *fp;
    
  fp = popen(cmd_full, "r");
    
  while (fgets(buf, BUFSIZE, fp) != NULL)
  {
    gtk_text_buffer_insert(buffer, &end, buf, -1);
  }
    
  fclose(fp);
}

static gboolean set_client_count(GtkTextBuffer **buffers)
{
  g_print(" entered ");

  char *cmd = "iw dev wlan0 station dump | grep 'Station' | wc -l";
  FILE *fp = popen(cmd, "r");
  
  int new_client_count;
  fscanf(fp, "%d", &new_client_count);
  fclose(fp);


  if (client_count != new_client_count)
  {
    int client_count_local = client_count;
    client_count = new_client_count;

    set_client_macs();

    if (new_client_count < client_count_local)
    {
      //find unneeded buffer
      //remove it
      //remove unneeded widgets
    } 
    else if (new_client_count > client_count_local)
    {
      *buffers = g_realloc(*buffers, new_client_count * sizeof(GtkTextBuffer*));  
      g_print("realloced");
      if (*buffers == NULL) 
      {
        g_print("realloc failed\n");
        return FALSE;
      }

      for (int i = client_count_local; i < new_client_count; i++) 
      {
        buffers[i] = gtk_text_buffer_new(NULL);
        
        if (buffers[i] == NULL) 
        {
          g_print("failed to set buffer\n");
          return FALSE;  
        }

        //set buffer
        set_buffer(buffers[i], macs[i]);
        set_widgets(buffers[i], macs[i]);
      }
      //draw one more widget
      //get macs, if buffers dont contain mac
      //draw new buffer
    }
  }
  else
  {
    //client count didnt change,
    //update buffers

    for (int i = 0; i < client_count; i++)
    {
      set_buffer(buffers[i], macs[i]);
    }
  }

  return TRUE;
}

static void activate (GtkApplication *app, gpointer user_data)
{
  GtkWidget *window;

  window = gtk_application_window_new (app);
  gtk_window_set_title(GTK_WINDOW(window), "Clients");
  gtk_window_maximize(GTK_WINDOW(window));
    
  grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(window), grid);

  GtkTextBuffer **buffers = g_new(GtkTextBuffer*, client_count);

  g_timeout_add(UPDATE_RATE, (GSourceFunc)set_client_count, buffers);
  gtk_widget_show_all(window);
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