#include <gtk/gtk.h>

void alert_popup(const char *message)
{
    GtkWidget* hello = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, message);

    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(hello), "This is secondary text with printf-style formatting: %d", 99);
    int response = gtk_dialog_run(GTK_DIALOG(hello));
    //printf("response was %d (OK=%d, DELETE_EVENT=%d)\n", response, GTK_RESPONSE_OK, GTK_RESPONSE_DELETE_EVENT);

    gtk_widget_destroy(hello);
}