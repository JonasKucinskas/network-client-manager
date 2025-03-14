#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include "headers/widgets.h"
#include "headers/api.h"
#include "headers/utils.h"

//init in main
GtkWidget *wan_view;
static GtkWidget *api_parameters_view;

enum
{
  COL_METHOD,
  COL_VALUE,
  NUM_COLS
};

Method methods[] = 
{
  {"auth.client",  NULL},
  {"auth.client.token",  NULL},
  {"auth.token.grant",  NULL},
  {"auth.token.revoke",  NULL},
  {"cmd.billing.newCycle", NULL},
  {"cmd.carrier.scan",  NULL},
  {"cmd.carrier.select",  NULL},
  {"cmd.channelPci.lock",  NULL},
  {"cmd.channelPci.scan",  NULL},
  {"cmd.config.apply",  NULL},
  {"cmd.config.discard",  NULL},
  {"cmd.config.restore",  NULL},
  {"cmd.mesh.discover", NULL},
  {"cmd.mesh.discover.result",  NULL},
  {"cmd.mesh.request", NULL},
  {"cmd.port.poe.disable",  NULL},
  {"cmd.port.poe.enable",  NULL},
  {"cmd.sendUssd", NULL},
  {"cmd.sms.get", NULL},
  {"cmd.sms.sendMessage",  NULL},
  {"cmd.starlink", NULL},
  {"cmd.ap", NULL},
  {"cmd.cellularModule.rescanNetwork",  NULL},
  {"cmd.cellularModule.reset", NULL},
  {"cmd.system.reboot", NULL},
  {"cmd.wan.cellular", NULL},
  {"cmd.wifi.connect", NULL},
  {"cmd.wifi.disconnect", NULL},
  {"cmd.wifi.forget", NULL},
  {"cmd.wifi.result", NULL},
  {"cmd.wifi.scan",  NULL},
  {"config.gpio", NULL},
  {"config.mesh",  NULL},
  {"config.speedfusionConnectProtect",  NULL},
  {"config.ssid.profile",  NULL},
  {"config.wan.connection", NULL},
  {"config.wan.connection.priority", NULL},
  {"info.firmware",  NULL},
  {"info.location",  NULL},
  {"info.time", NULL},
  {"status.cellularModule.temperature", NULL},
  {"status.client", NULL},
  {"status.extap.mesh", NULL},
  {"status.extap.mesh.link", NULL},
  {"status.gpio.input", NULL},
  {"status.gpio.output", NULL},
  {"status.lan.profile", NULL},
  {"status.pepvpn", NULL},
  {"status.wan.connection", NULL},
  {"status.wan.connection.allowance", NULL}
};

static GtkTreeModel* create_and_fill_model(void)
{
  GtkTreeStore *treestore;
  GtkTreeIter toplevel, child;

  treestore = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);

  //todo will break, when adding parameters
  int method_count = sizeof(methods) / sizeof(methods[0]);

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
  g_print("Row activated: %s\n", methods[row_index].name);

  JsonParser *parser = json_parser_new();
  GError *error = NULL;

  struct MemoryStruct chunk;
  chunk.memory = malloc(1);  
  chunk.size = 0;

  api_call(&chunk, methods[row_index].name, NULL);

  if (chunk.size == 0)
  {
    //api returned an error.
    return;
  }

  json_parser_load_from_data(parser, chunk.memory, chunk.size, &error);
  
  //no longer needed
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
    if(json_code == 401)//unauthorized
    {
      //writes new cookie if current cookie is wrong or does not exist
      gboolean saved_cookie = api_save_auth_cookie();

      if (saved_cookie == FALSE)
      {
        alert_popup("Authorization failed", "Failed to re-authorize.");
      }
      alert_popup("", "Authorization failed, re-authorized.");
    }
    else if(json_code == 999)//unknown function
    {
      alert_popup("", "This method does not exist");
    }
    else if(json_code == 404)//unsupported request
    {
      alert_popup("", "Parameters not set for this method.");
      
      //todo will break, when adding parameters
      int method_count = sizeof(methods) / sizeof(methods[0]);

      show_parameter_dialog(methods, method_count, row_index);
    }
    else
    {
      alert_popup("", "Unknown return code.");
    }

    //json returned error.
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