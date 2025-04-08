#ifndef WAN_H
#define WAN_H
#include <gtk/gtk.h>
#include "utils.h"

extern GtkWidget *wan_view;
extern MethodContainer *method_container;

void init_wan_page();
void remove_row_from_model(const char *name_to_remove);
void add_row_to_model(const char *name_to_add);

#endif