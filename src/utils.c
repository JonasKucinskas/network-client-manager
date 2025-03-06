#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>
#include "headers/utils.h"
#include "headers/widgets.h"
#include "headers/api.h"

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

void traverse_json(JsonNode *node, GtkTreeStore *treestore, GtkTreeIter *iter) 
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
        
        traverse_json(value, treestore, &child);
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
        traverse_json(arrayElement, treestore, iter);
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

int method_count(char **methods)
{
  int count = 0;
  
  while (methods[count] != NULL) 
  {
    count++;
  }
  
  return count;
}

static gchar* json_get_value(JsonNode *root, const char* json_path)
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

gboolean json_error_parse(JsonNode *root)
{
  gchar *value = json_get_value(root, "$.stat");

  if(strcmp(value, "\"fail\"") == 0)
  {
    value = json_get_value(root, "$.message");
    g_print(value);

    if(strcmp(value, "\"Unauthorized\"") == 0)
    {
      //writes new cookie if this cookie is wrong.
      //if cookie does not exist, it is created in api_call()
      api_save_auth_cookie();
      alert_popup(value, "Authorization failed, re-authorized.");
    }
    else if(strcmp(value, "\"unknown function\"") == 0)
    {
      alert_popup(value, "This method does not exist");
    }
    else if(strcmp(value, "\"Unsupported request\"") == 0)
    {
      alert_popup(value, "Parameter not set");
    }

    return TRUE;
  }
  
  g_free(value);
  return FALSE;
}