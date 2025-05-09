#include <gtk/gtk.h>
#include <json-glib/json-glib.h>
#include "headers/widgets.h"
#include "headers/api.h"
#include "headers/utils.h"

//init in main
GtkWidget *wan_view;
char *search_text;
GtkTreeModel *filter;

enum
{
    COL_METHOD,
    COL_VALUE,
    NUM_COLS
};

MethodContainer *method_container;

static GtkTreeModel* create_and_fill_model(void)
{
    GtkTreeIter toplevel;

    GtkTreeStore *treestore = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING);

    for(size_t i = 0; i < method_container->method_count; i++)
    {
        gtk_tree_store_append(treestore, &toplevel, NULL);
        gtk_tree_store_set(treestore, &toplevel, COL_METHOD, method_container->methods[i].name, COL_VALUE, "", -1);
    }

    return GTK_TREE_MODEL(treestore);
}

void remove_row_from_model(const char *name_to_remove)
{       
    GtkTreeIter iter; 
    GtkTreeModel* tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wan_view));
    tree_model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(tree_model));

    gtk_tree_model_get_iter_first(tree_model, &iter);

    while (TRUE) 
    {
        gchar *method_name;
        gtk_tree_model_get(tree_model, &iter, 0, &method_name, -1);

        if (strcmp(method_name, name_to_remove) == 0)
        {
            gtk_tree_store_remove(GTK_TREE_STORE(tree_model), &iter);
            g_free(method_name);
            return;
        }
        gboolean is_valid = gtk_tree_model_iter_next(tree_model, &iter);

        if (!is_valid)
        {
            return;
        }
    }
}

void add_row_to_model(const char *name_to_add)
{       
    GtkTreeIter iter; 
    GtkTreeModel* tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(wan_view));
    tree_model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(tree_model));



    gtk_tree_store_append(GTK_TREE_STORE(tree_model), &iter, NULL);
    gtk_tree_store_set(GTK_TREE_STORE(tree_model), &iter, COL_METHOD, name_to_add, COL_VALUE, "", -1);
}

static gboolean filter_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data) 
{
    const gchar *row_name;
    gtk_tree_model_get(model, iter, COL_METHOD, &row_name, -1);

    if (search_text == NULL || *search_text == '\0')
        return TRUE;

    gchar *row_lower = g_utf8_strdown(row_name, -1);
    gchar *search_lower = g_utf8_strdown(search_text, -1);

    gboolean visible = g_strrstr(row_lower, search_lower) != NULL;

    g_free(row_lower);
    g_free(search_lower);

    return visible;
}

void on_search_changed_wan(GtkEntry *entry, gpointer user_data) 
{
    g_free(search_text); 
    search_text = g_strdup(gtk_entry_get_text(entry));

    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(filter)); 
}


void create_model(GtkWidget *view)
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
    *    model column that contains the first name */
    gtk_tree_view_column_add_attribute(col, renderer, "text", COL_METHOD);

    /* --- Column #2 --- */
    col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Value");

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, renderer, TRUE);

    /* connect 'text' property of the cell renderer to
     *    model column that contains the last name */
    gtk_tree_view_column_add_attribute(col, renderer, "text", COL_VALUE);


    model = create_and_fill_model();

    filter = gtk_tree_model_filter_new(model, NULL);
    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), filter_func, NULL, NULL);

    gtk_tree_view_set_model(GTK_TREE_VIEW(view), filter);
    g_object_unref(filter);

    g_object_unref(model); /* destroy model automatically with wan_view */

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_SINGLE);
}

void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) 
{
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model));

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
    Method *selected_method = &method_container->methods[row_index];

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
    int json_code = get_json_return_code(root);

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

void init_wan_page()
{
    parse_json_into_memory(&method_container);
 
    g_signal_connect(wan_view, "row-activated", G_CALLBACK(on_row_activated), NULL);
    create_model(wan_view);
    gtk_widget_show_all(wan_view);
}