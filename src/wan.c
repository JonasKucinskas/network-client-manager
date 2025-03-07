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

  treestore = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);

  char **methods = (char *[])
  { 
    "auth.client",
    "auth.client.token",
    "cmd.carrier.scan",
    "cmd.mesh.discover.result",
    "cmd.mesh.discover",
    "cmd.sms.get",
    "cmd.ap",
    "cmd.wifi.result",
    "cmd.wifi.scan",
    "config.mesh",
    "config.speedfusionConnectProtect",
    "config.ssid.profile",
    "config.wan.connection",
    "info.firmware",
    "info.location",
    "info.time",
    "status.extap.mesh",
    "status.extap.mesh.link",
    "status.gpio.input",
    "status.gpio.output",
    "status.lan.profile",
    "status.pepvpn",
    "status.wan.connection",
    "status.wan.connection.allowance",
    NULL
    }; 


  for(int i = 0; i < method_count(methods); i++)
  {
    gtk_tree_store_append(treestore, &toplevel, NULL);
    gtk_tree_store_set(treestore, &toplevel, COL_METHOD, methods[i], COL_VALUE, "", -1);
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
  
  gboolean got_iter = gtk_tree_model_get_iter(model, &iter, path);

  if (!got_iter) 
  {
    return;
  }
    
  GtkTreeIter parent, child;
  gboolean has_parent = gtk_tree_model_iter_parent(model, &parent, &iter);
  gboolean has_children = gtk_tree_model_iter_children(model, &child, &iter);
  gboolean is_expanded = gtk_tree_view_row_expanded(tree_view, path);

  if (has_parent)
  {
    //user cliked on nested line which is not an api method.
    //expand/collapse that row
    toggle_row_expansion(tree_view, path, !is_expanded, FALSE); 
    return;
  }
  
  if (has_children)
  {
    //if used clicked on api method row and it already has data filled in, 
    //expand/collapse that row
    toggle_row_expansion(tree_view, path, !is_expanded, TRUE); 
    return;
  }


  gchar *method_name;
  
  gtk_tree_model_get(model, &iter, COL_METHOD, &method_name, -1);
  g_print("Row activated: %s\n", method_name);

  JsonParser *parser = json_parser_new();
  GError *error = NULL;

  struct MemoryStruct chunk;
  chunk.memory = malloc(1);  
  chunk.size = 0;

  api_call(&chunk, method_name, NULL);

  json_parser_load_from_data(parser, chunk.memory, chunk.size, &error);
  
  free(chunk.memory);
  
  if(error)
  {
    g_print("Unable to parse: %s\n", error->message);
        
    g_error_free(error);
    g_object_unref(parser);
    g_free(method_name);
    return;
  }
  JsonNode *root = json_parser_get_root(parser);

  //api returns code 200 always, so I have to parse json for actual error messages :(((
  gboolean json_error = json_error_parse(root);
  
  if(json_error)
  {
    g_print("api returned error");
    json_node_free(root); 
    g_free(method_name); 
    return;
  }
  
  json_tree_draw(root, GTK_TREE_STORE(model), &iter);
  
  //expand the row after drawing the tree.
  gtk_tree_view_expand_row(tree_view, path, FALSE);

  json_node_free(root);
  g_free(method_name);
  g_object_unref(parser);
}

void draw_tree_view()
{
  //set wan_view and model;
  g_signal_connect(wan_view, "row-activated", G_CALLBACK(on_row_activated), NULL);
  create_model();
  gtk_widget_show_all(wan_view);
}