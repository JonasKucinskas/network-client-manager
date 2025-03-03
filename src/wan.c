#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include "headers/widgets.h"
#include "headers/api.h"
#include "headers/utils.h"

//init in main
GtkWidget *wan_view;

enum
{
  COL_METHOD,
  COL_VALUE,
  NUM_COLS
};

static GtkTreeModel* create_and_fill_model(void)
{
  GtkTreeStore *treestore;
  GtkTreeIter toplevel, child;

  treestore = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);

  char **methods = (char *[]){ "status.wan.connection", "info.firmware", "info.location", "config.ssid.profile"}; 
  
  for(int i = 0; i < 4; i++)
  {
    gtk_tree_store_append(treestore, &toplevel, NULL);
    gtk_tree_store_set(treestore, &toplevel, COL_METHOD, methods[i], COL_VALUE, "", -1);

    //loop for nested structure, append as child
    /*
    gtk_tree_store_append(treestore, &child, &toplevel);
    gtk_tree_store_set(treestore, &child,
                     COL_FIRST_NAME, "Janinita",
                     COL_LAST_NAME, "Average",
                     COL_YEAR_BORN, (guint) 1985,
                     -1);
    */
  }
  return GTK_TREE_MODEL(treestore);
}

static void create_model()
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, "Method");
  gtk_tree_view_append_column(GTK_TREE_VIEW(wan_view), col);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
  *  model column that contains the first name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_METHOD);

  /* --- Column #2 --- */
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "Value");

  gtk_tree_view_append_column(GTK_TREE_VIEW(wan_view), col);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the last name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_VALUE);


  model = create_and_fill_model();

  gtk_tree_view_set_model(GTK_TREE_VIEW(wan_view), model);

  g_object_unref(model); /* destroy model automatically with wan_view */

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(wan_view)), GTK_SELECTION_SINGLE);
}

void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) 
{
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;
  
  if (gtk_tree_model_get_iter(model, &iter, path)) 
  {
    gchar *method_name;
    
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  
    chunk.size = 0;

    gtk_tree_model_get(model, &iter, COL_METHOD, &method_name, -1);
    g_print("Row activated: %s %s\n", method_name);
    
    char *data = "username=admin&password=Admin12345";
    
    //TODO login once
    api_save_auth_cookie("login", data);
    api_call(&chunk, method_name, NULL);
    
    JsonParser *parser = json_parser_new();
    JsonNode *root;
    GError *error = NULL;

    json_parser_load_from_data(parser, chunk.memory, chunk.size, &error);

    if (error)
    {
      g_print ("Unable to parse: %s\n", error->message);
      g_error_free (error);
      g_object_unref (parser);
      return;
    }
 
    JsonReader *reader = json_reader_new (json_parser_get_root (parser));
    char** members = json_reader_list_members(reader);

    int i = 0;
    while (members[i] != 0) 
    {
      char *m = members[i];
      
      json_reader_read_member (reader, members[i]);
      const char *value = json_reader_get_string_value (reader);
      json_reader_end_member (reader);
      
      if (m == "message") 
      {
        alert_popup(value);
        //g_print("parse member %s\n", members[i]);
        //g_print("parse value %s\n", value);
        if (value == "Unauthorized")
        {
        }
      }

      if(m == "stat")
      {
        if (value == "fail")
        {
        }
      }

      if(m == "code")
      {
        if (value == "401")
        {
          //no auth
        }
      }
      i++;
    }

    g_strfreev(members);
    g_object_unref (reader);
    g_object_unref (parser);

    gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &iter);
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COL_METHOD, "", COL_VALUE, chunk.memory, -1);
  }
}

void draw_tree_view()
{
  //set wan_view and model;
  g_signal_connect(wan_view, "row-activated", G_CALLBACK(on_row_activated), NULL);
  create_model();
  gtk_widget_show_all(wan_view);
}