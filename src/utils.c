#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>
#include "headers/utils.h"
#include "headers/widgets.h"
#include "headers/api.h"
#include "headers/wan.h"

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  
  if(mem->memory == NULL) 
  {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

//appends json structure to existing tree
void json_tree_draw(JsonNode *node, GtkTreeStore *treestore, GtkTreeIter *iter) 
{
  GtkTreeIter child;
  switch (json_node_get_node_type(node)) 
  {
    case JSON_NODE_OBJECT: 
    {
      JsonObject *object = json_node_get_object(node);
      GList *members = json_object_get_members(object);

      for (GList *l = members; l != NULL; l = l->next) 
      {
        const gchar *key = (const gchar *) l->data;
        JsonNode *value = json_object_get_member(object, key);

        gtk_tree_store_append(treestore, &child, iter);
        //0th column, obj name value:
        gtk_tree_store_set(treestore, &child, 0, key, -1);
        
        json_tree_draw(value, treestore, &child);
      }

      g_list_free(members);
      break;
    }
    case JSON_NODE_ARRAY: 
    {
      JsonArray *array = json_node_get_array(node);
      guint length = json_array_get_length(array);
      
      for (guint i = 0; i < length; i++) 
      {
        JsonNode *arrayElement = json_array_get_element(array, i);
        
        GValue gvalue = G_VALUE_INIT;
        json_node_get_value(arrayElement, &gvalue);
        gchar *strVal = g_strdup_value_contents(&gvalue);

        gtk_tree_store_append(treestore, &child, iter);
        //0th column, obj name value:
        gtk_tree_store_set(treestore, &child, 0, strVal, -1);

        g_value_unset(&gvalue);
        g_free(strVal);
        json_tree_draw(arrayElement, treestore, iter);
      }
      break;
    }
    case JSON_NODE_VALUE: 
    {
      GValue gvalue = G_VALUE_INIT;
      json_node_get_value(node, &gvalue);

      gchar *strVal = g_strdup_value_contents(&gvalue);
      
      gtk_tree_store_append(treestore, iter, &child);
      //1st column, property value:
      gtk_tree_store_set(treestore, iter, 1, strVal, -1);

      g_value_unset(&gvalue);
      g_free(strVal);
      break;
    }
    default:
      printf("Unknown type\n");
      break;
  }
}

static gchar* json_get_string(JsonNode *root, const char* json_path)
{
  JsonPath *path = json_path_new();
  
  GError *error = NULL;
  json_path_compile(path, json_path, &error);
  
  JsonNode *path_match = json_path_match(path, root);
  JsonArray *array = json_node_get_array(path_match);
  JsonNode *arrayElement = json_array_get_element(array, 0);
      
  GValue gvalue = G_VALUE_INIT;
  json_node_get_value(arrayElement, &gvalue);
  gchar *strVal = g_strdup_value_contents(&gvalue);

  return strVal;
}

static int json_get_int(JsonNode *root, const char* json_path)
{
  JsonPath *path = json_path_new();
 
  GError *error = NULL;
  json_path_compile(path, json_path, &error);
 
  JsonNode *path_match = json_path_match(path, root);
  JsonArray *array = json_node_get_array(path_match);
  JsonNode *arrayElement = json_array_get_element(array, 0);
  
  return json_node_get_int(arrayElement); 
}

//returns code embedded in api return
int json_error_parse(JsonNode *root)
{
  gchar *status = json_get_string(root, "$.stat");
  int code;

  if(strcmp(status, "\"fail\"") == 0)
  {
    code = json_get_int(root, "$.code");
  }
  else 
  {
    code = 200;
  }
  
  //if stat == 'ok', json just does not return a code?
  g_free(status);
  return code;
}

void toggle_row_expansion(GtkTreeView *tree_view, GtkTreePath *path, gboolean expand, gboolean expand_all) 
{
  if (expand) 
  {
    gtk_tree_view_expand_row(tree_view, path, expand_all);
  } 
  else 
  {
    gtk_tree_view_collapse_row(tree_view, path);
  }
}

void handle_json_error(int error_code, int row_index)
{
  if(error_code == 401)//unauthorized
    {
      //writes new cookie if current cookie is wrong or does not exist
      gboolean saved_cookie = api_save_auth_cookie();

      if (saved_cookie == FALSE)
      {
        alert_popup("Authorization failed", "Failed to re-authorize.");
      }
      alert_popup("", "Authorization failed, re-authorized.");
    }
    else if(error_code == 999)//unknown function
    {
      alert_popup("", "This method does not exist");
    }
    else if(error_code == 404)//unsupported request
    {
      alert_popup("", "Parameters not set for this method.");
      
      show_parameter_dialog(row_index);
    }
    else
    {
      alert_popup("", "Unknown return code.");
    }
}

void make_post_data_from_object(char *str, Method *method)
{
  str[0] = '\0';
  for (int i = 0; i < method->param_count; i++)
  {
    char *name = method->parameters[i].name;
    char *value = method->parameters[i].value;

    strcat(str, name);
    strcat(str, "=");
    strcat(str, value);

    //if not last param:
    if (i + 1 < method->param_count)
    {
      strcat(str, "&");
    }
  }
}