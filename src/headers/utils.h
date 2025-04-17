#ifndef UTILS_H
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>

#define UTILS_H

extern gchar *base_url;
extern gchar *username;
extern gchar *password;


struct MemoryStruct 
{
  char *memory;
  size_t size;
};

typedef struct {
    char *name;
    char *value; 
} Parameter;

typedef struct {
    const gchar *name;  
    size_t param_count;
    Parameter *parameters; 
} Method;

typedef struct {
    size_t method_count;
    Method *methods;
} MethodContainer;

typedef struct {
    GtkWidget *name;
    GtkWidget *value;
} ParameterWidgets;

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
void json_tree_draw(JsonNode *node, GtkTreeStore *treestore, GtkTreeIter *iter);
int get_json_return_code(JsonNode *root);
void toggle_row_expansion(GtkTreeView *tree_view, GtkTreePath *path, gboolean expand, gboolean expand_all);
gchar* json_get_value(JsonNode *root, const char* json_path);
void handle_json_error(int error_code, int row_index);
void make_post_data_from_object(char **str, Method *method);
void parse_json_into_memory(MethodContainer **method_container);
gboolean write_params_json(Method *method);
void remove_method_from_json(int method_index);
void add_method_to_memory(const char *method_name);
void write_method_to_json(const char *method_name);
gboolean remove_method_from_memory(int method_index);
int find_method_index(const char* method_name);
gboolean write_user_details_to_json();

#endif