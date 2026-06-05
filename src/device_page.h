#ifndef DEVICE_PAGE_H
#define DEVICE_PAGE_H

#include <gtk/gtk.h>
#include "input_detection.h"

/*
 * The ButtonData_t struct holds a reference to a mouse device and GtkStack.
 * Mainly used to pass through to the populate_editor function for the editor page.
 */
typedef struct 
{
    MouseDevice_t device;
    GtkStack *stack;
} ButtonData_t;

/// @brief Builds a GUI page by converting each mouse device info into a clickable button.
/// @param mice An array of mouse devices.
/// @param count Length of the array.
/// @param stack The GTK Stack for the main program.
/// @return Returns a widget to the parent that holds all of the GUI elements of the device page.
GtkWidget *build_device_page(MouseDevice_t *mice, int count, GtkStack *stack);

#endif