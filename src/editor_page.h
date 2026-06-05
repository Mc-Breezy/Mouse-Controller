#ifndef EDITOR_PAGE_H
#define EDITOR_PAGE_H

#include <gtk/gtk.h>
#include "input_detection.h"

typedef struct 
{
    GtkLabel *name_label;
    GtkLabel *path_label;
    GtkLabel *ids_label;
    GtkBox *buttons_rows_box;
    GtkButton *grab_btn;
    GtkScale *sensitivity_scale;
    GPtrArray *remap_rows;
    MouseDevice_t current_device;
    gboolean grabbed;
} EditorWidgets_t;

/// @brief Builds a GUI page for editing the properties of a mouse
/// @param stack The main stack of the program
/// @param window The window for the program
/// @return Returns a widget to the parent that holds all of the GUI elements of the editor page
GtkWidget *build_editor_page(GtkStack *stack, GtkWindow *window);

/// @brief Populates the information of the editor page so that the user can edit their device
/// @param stack The main stack of the program
/// @param device The information device being edited
void populate_editor(GtkStack *stack, MouseDevice_t *device);

#endif