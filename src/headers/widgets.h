#ifndef WIDGETS_H
#define WIDGETS_H
#include <gtk/gtk.h>
#include "utils.h"

void alert_popup(const char *header, const char *body);
void ask_for_parameters();
void show_parameter_dialog(Method methods[], int method_count, int row_index);

#endif