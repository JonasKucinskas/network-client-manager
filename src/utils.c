#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>
#include "headers/utils.h"

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
        JsonNode *value = json_array_get_element(array, i);
        //g_print("%d: ", i);
        traverse_json(value, treestore, iter);
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

      free(strVal);
      break;
    }
    default:
      printf("Unknown type\n");
      break;
  }
}