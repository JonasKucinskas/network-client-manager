#ifndef WIDGETS_H
#define WIDGETS_H
#include <gtk/gtk.h>
#include "utils.h"

void alert_popup(const char *header, const char *body);
void show_parameter_dialog(int row_index);

#endif