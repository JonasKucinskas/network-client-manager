#ifndef UTILS_H
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>

#define UTILS_H

struct MemoryStruct 
{
  char *memory;
  size_t size;
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
void traverse_json(JsonNode *node, GtkTreeStore *treestore, GtkTreeIter *iter);
int method_count(char **methods);
gboolean json_error_parse(JsonNode *root);

#endif