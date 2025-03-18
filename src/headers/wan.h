#ifndef WAN_H
#define WAN_H
#include <gtk/gtk.h>
#include "utils.h"

extern GtkWidget *wan_view;
extern Method methods[];
extern size_t method_count;

void draw_tree_view();

#endif