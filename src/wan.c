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

Method methods[] = 
{
  {"auth.client", 0, NULL},
  {"auth.client.token", 0, NULL},
  {"auth.token.grant", 0, NULL},
  {"auth.token.revoke", 0, NULL},
  {"cmd.billing.newCycle", 0, NULL},
  {"cmd.carrier.scan", 0, NULL},
  {"cmd.carrier.select", 0, NULL},
  {"cmd.channelPci.lock", 0, NULL},
  {"cmd.channelPci.scan", 0, NULL},
  {"cmd.config.apply", 0, NULL},
  {"cmd.config.discard", 0, NULL},
  {"cmd.config.restore", 0, NULL},
  {"cmd.mesh.discover", 0, NULL},
  {"cmd.mesh.discover.result", 0, NULL},
  {"cmd.mesh.request", 0, NULL},
  {"cmd.port.poe.disable", 0, NULL},
  {"cmd.port.poe.enable", 0, NULL},
  {"cmd.sendUssd", 0, NULL},
  {"cmd.sms.get", 0, NULL},
  {"cmd.sms.sendMessage", 0, NULL},
  {"cmd.starlink", 0, NULL},
  {"cmd.ap", 0, NULL},
  {"cmd.cellularModule.rescanNetwork", 0, NULL},
  {"cmd.cellularModule.reset", 0, NULL},
  {"cmd.system.reboot", 0, NULL},
  {"cmd.wan.cellular", 0, NULL},
  {"cmd.wifi.connect", 0, NULL},
  {"cmd.wifi.disconnect", 0, NULL},
  {"cmd.wifi.forget", 0, NULL},
  {"cmd.wifi.result", 0, NULL},
  {"cmd.wifi.scan", 0, NULL},
  {"config.gpio", 0, NULL},
  {"config.mesh", 0, NULL},
  {"config.speedfusionConnectProtect", 0, NULL},
  {"config.ssid.profile", 0, NULL},
  {"config.wan.connection", 0, NULL},
  {"config.wan.connection.priority", 0, NULL},
  {"info.firmware", 0, NULL},
  {"info.location", 0, NULL},
  {"info.time", 0, NULL},
  {"status.cellularModule.temperature", 0, NULL},
  {"status.client", 0, NULL},
  {"status.extap.mesh", 0, NULL},
  {"status.extap.mesh.link", 0, NULL},
  {"status.gpio.input", 0, NULL},
  {"status.gpio.output", 0, NULL},
  {"status.lan.profile", 0, NULL},
  {"status.pepvpn", 0, NULL},
  {"status.wan.connection", 0, NULL},
  {"status.wan.connection.allowance", 0, NULL}
};

size_t method_count = 0;

static GtkTreeModel* create_and_fill_model(void)
{
  GtkTreeStore *treestore;
  GtkTreeIter toplevel, child;

  treestore = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);

  method_count = sizeof(methods) / sizeof(methods[0]);

  for(int i = 0; i < method_count; i++)
  {
    gtk_tree_store_append(treestore, &toplevel, NULL);
    gtk_tree_store_set(treestore, &toplevel, COL_METHOD, methods[i].name, COL_VALUE, "", -1);
  }

  return GTK_TREE_MODEL(treestore);
}

static void create_model(GtkWidget *view)
{
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;
  GtkTreeModel *model;

  col = gtk_tree_view_column_new();

  gtk_tree_view_column_set_title(col, "Method");
  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
  *  model column that contains the first name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_METHOD);

  /* --- Column #2 --- */
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "Value");

  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  /* connect 'text' property of the cell renderer to
   *  model column that contains the last name */
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_VALUE);


  model = create_and_fill_model();

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);

  g_object_unref(model); /* destroy model automatically with wan_view */

  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_SINGLE);
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
    //expand/collapse that row with all its children
    toggle_row_expansion(tree_view, path, !is_expanded, TRUE); 
    return;
  }
  
  int row_index = gtk_tree_path_get_indices(path)[0];
  Method *selected_method = &methods[row_index];

  g_print("Row activated: %s\n", selected_method->name);

  JsonParser *parser = json_parser_new();
  GError *error = NULL;


  struct MemoryStruct chunk;
  chunk.memory = malloc(1);  
  chunk.size = 0;

  api_call(&chunk, selected_method);

  if (chunk.size == 0)
  {
    //api returned an error.
    return;
  }

  json_parser_load_from_data(parser, chunk.memory, chunk.size, &error);
  
  //no longer needed, data is in parser.
  free(chunk.memory);
  
  if(error)
  {
    alert_popup("Error while parsing json", error->message);

    g_error_free(error);
    g_object_unref(parser);
    return;
  }

  JsonNode *root = json_parser_get_root(parser);

  //api returns code 200 always, so I have to parse json for actual error messages :(((
  int json_code = json_error_parse(root);
  
  if(json_code != 200)
  {
    handle_json_error(json_code, row_index);
    json_node_free(root); 
    return;
  }
  
  json_tree_draw(root, GTK_TREE_STORE(model), &iter);
  
  //expand the row after drawing the tree.
  gtk_tree_view_expand_row(tree_view, path, FALSE);

  json_node_free(root);
  g_object_unref(parser);
}

void draw_tree_view()
{
  //set wan_view and model;
  g_signal_connect(wan_view, "row-activated", G_CALLBACK(on_row_activated), NULL);
  create_model(wan_view);
  gtk_widget_show_all(wan_view);
}