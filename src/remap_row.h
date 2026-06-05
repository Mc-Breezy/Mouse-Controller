#ifndef REMAP_ROW_H
#define REMAP_ROW_H

#include <gtk/gtk.h>
#include "input_detection.h"

/*
 * The following RemapRow_t struct contains elements for a button row.
 * It contains information relating to a button with the MouseButtons_t struct,
 * the label showing the name of the current binding, the button to listen for key presses
 * to assign a new value, and a boolean telling whether to row is listening for an input.
 */
typedef struct 
{
    MouseButtons_t *button;
    GtkLabel *binding_label;
    GtkButton *listen_btn;
    gboolean listening;
} RemapRow_t;

/// @brief Constructs a new remap row box that allows the user to remap a mouse button.
/// @param button The mouse button info to build off
/// @param out_row The pointer to the row that is returned
/// @return Returns a box that contains all of the widgets for the row.
GtkWidget *build_remap_row(MouseButtons_t *button, RemapRow_t **out_row);

/// @brief Converts an evdev keycode to a string name
/// @param code An evdev key code 
/// @return A string of the name of the code typed out (IE: Left Click for BTN_LEFT)
const char *keycode_name(int code);

#endif