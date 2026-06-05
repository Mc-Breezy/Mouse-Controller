#include "editor_page.h"
#include "remap_row.h"
#include "remap.h"
#include "utility.h"
#include <libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <stdio.h>
#include <string.h>

/// @brief Converts a gdk key value to its corresponding evdev key value counterpart.
/// @param keyval The gdk key value to be converted.
/// @return An evdev key code. A default case returns -1.
static int gdk_keyval_to_evdev(guint keyval)
{
    switch (keyval)
    {
        case GDK_KEY_a: return KEY_A; case GDK_KEY_b: return KEY_B;
        case GDK_KEY_c: return KEY_C; case GDK_KEY_d: return KEY_D;
        case GDK_KEY_e: return KEY_E; case GDK_KEY_f: return KEY_F;
        case GDK_KEY_g: return KEY_G; case GDK_KEY_h: return KEY_H;
        case GDK_KEY_i: return KEY_I; case GDK_KEY_j: return KEY_J;
        case GDK_KEY_k: return KEY_K; case GDK_KEY_L: return KEY_L;
        case GDK_KEY_m: return KEY_M; case GDK_KEY_n: return KEY_N;
        case GDK_KEY_o: return KEY_O; case GDK_KEY_p: return KEY_P;
        case GDK_KEY_q: return KEY_Q; case GDK_KEY_r: return KEY_R;
        case GDK_KEY_s: return KEY_S; case GDK_KEY_t: return KEY_T;
        case GDK_KEY_u: return KEY_U; case GDK_KEY_v: return KEY_V;
        case GDK_KEY_w: return KEY_W; case GDK_KEY_x: return KEY_X;
        case GDK_KEY_y: return KEY_Y; case GDK_KEY_z: return KEY_Z; 
        case GDK_KEY_0: return KEY_0; case GDK_KEY_1: return KEY_1;
        case GDK_KEY_2: return KEY_2; case GDK_KEY_3: return KEY_3;
        case GDK_KEY_4: return KEY_4; case GDK_KEY_5: return KEY_5;
        case GDK_KEY_6: return KEY_6; case GDK_KEY_7: return KEY_7;
        case GDK_KEY_8: return KEY_8; case GDK_KEY_9: return KEY_9;
        case GDK_KEY_Return: return KEY_ENTER;
        case GDK_KEY_Escape: return KEY_ESC;
        case GDK_KEY_space: return KEY_SPACE;
        case GDK_KEY_BackSpace: return KEY_BACKSPACE;
        case GDK_KEY_Tab: return KEY_TAB;
        case GDK_KEY_F1: return KEY_F1; case GDK_KEY_F2: return KEY_F2;
        case GDK_KEY_F3: return KEY_F3; case GDK_KEY_F4: return KEY_F4;
        case GDK_KEY_F5: return KEY_F5; case GDK_KEY_F6: return KEY_F6;
        case GDK_KEY_F7: return KEY_F7; case GDK_KEY_F8: return KEY_F8;
        case GDK_KEY_F9: return KEY_F9; case GDK_KEY_F10: return KEY_F10;
        case GDK_KEY_F11: return KEY_F11; case GDK_KEY_F12: return KEY_F12;
        default: return -1;
    }
}

/// @brief A callback function that checks if any remappable button is listening and 
/// waiting for an input from the keyboard to set the new value of the button. 
/// @param controller The GtkEvenControllerKey passed through the callback.
/// @param keyval The value of the key that was pressed.
/// @param keycode The keycode of the key that pressed.
/// @param state The state or modifier that was used when the key was pressed.
/// @param user_data The user data that was passed through the callback (EditorWidgets_t in this case).
/// @return A boolean that is FALSE if a button was not listening and TRUE if a button was listening.
static gboolean on_key_pressed(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
    //Retrieve the EditorWidgets_t from the user_data that was passed through the callback when it was setup.
    EditorWidgets_t *widgets = user_data;
    //If the widget structure is NULL, return FALSE.
    if (!widgets->remap_rows) return FALSE;

    //For each of the rows(buttons) in the editor window
    for (guint i = 0; i < widgets->remap_rows->len; i++)
    {
        //Get the row
        RemapRow_t *row = g_ptr_array_index(widgets->remap_rows, i);

        //Check if the button is listening
        if (row->listening)
        {
            //Convert the gdk keyval to and evdev keyval
            int evdev_code = gdk_keyval_to_evdev(keyval);
            //If the evdev code is less than zero, it was a default case. So return
            if (evdev_code < 0) return TRUE;

            //Set the new remapped code to the evdev code value
            row->button->remap_code = evdev_code;
            //Set the label
            label_set_safe(row->binding_label, libevdev_event_code_get_name(EV_KEY, evdev_code));
            //gtk_label_set_text(row->binding_label, libevdev_event_code_get_name(EV_KEY, evdev_code));
            
            //Set listening to false
            row->listening = FALSE;
            //Set the label for the button back to default
            gtk_button_set_label(row->listen_btn, "Listen");
            gtk_widget_remove_css_class(GTK_WIDGET(row->listen_btn), "listening");
        
            return TRUE;
        }
    }

    return FALSE;
}

/// @brief Sets the device list visible
/// @param button The GtkButton associated with the callback 
/// @param stack The main stack of the program
static void on_back_clicked(GtkButton *button, gpointer stack)
{
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "device-list");
}

/// @brief Callback for when the grab button is clicked. It sets the mouse sensitivity 
/// and applys an remaps or disables them.
/// @param btn The button tied to the callback.
/// @param user_data The user data passed through the callback (struct EditorWidgets_t).
static void on_grab_clicked(GtkButton *btn, gpointer user_data)
{
    //Cast the user data to the EditorWidgets_t struct
    EditorWidgets_t *widgets = user_data;

    //Check if the device is not already grabbed
    if (!widgets->grabbed)
    {
        //Set the sensitivity of the current device equal to what the GtkScale value is
        widgets->current_device.sensitivity = (float)gtk_range_get_value(GTK_RANGE(widgets->sensitivity_scale));

        //Attempt to apply button remaps
        if (apply_remaps(&widgets->current_device) == 0)
        {
            //Set grabbed equal to true
            widgets->grabbed = TRUE;
            //Set the label to ungrab
            gtk_button_set_label(btn, "Ungrab Device");
        }
    }
    else
    {
        //Disable the remaps
        disable_remaps(&widgets->current_device);
        //Set grabbed to false
        widgets->grabbed = FALSE;
        //Set the label back to default
        gtk_button_set_label(btn, "Grab Device");
    }
}

/// @brief Frees the data related to the EditorWidgets
/// @param data The EditorWidgets_t struct passed through the callback
static void editor_widgets_free(gpointer data)
{
    //Cast the data to EditorWidgets_t
    EditorWidgets_t *widgets = data;
    //Unref and free the remap_rows array
    g_ptr_array_unref(widgets->remap_rows);
    //Free the widgets struct
    g_free(widgets);
}

GtkWidget *build_editor_page(GtkStack *stack, GtkWindow *window)
{
    //Outer box that holds all of the other elements
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    //Allocate a new EditorWidgets_t data struct
    EditorWidgets_t *widgets = g_new0(EditorWidgets_t, 1);
    //Allocate space for the remap_rows array
    widgets->remap_rows = g_ptr_array_new();
    //The device should not be grabbed by default
    widgets->grabbed = FALSE;

    //Construct a new GtkEventController
    GtkEventController *key_ctrl = gtk_event_controller_key_new();
    //Add the event controller to the window, so it can take input for the entire application
    gtk_widget_add_controller(GTK_WIDGET(window), key_ctrl);
    //Setup the key-pressed callback linking to the on_key_pressed function. Pass through
    //the editor widgets.
    g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), widgets);

    //A box to hold the back button, labels, and most of the elements near the top.
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);

    //Construct a back button. This is static and does not need to be stored or updated
    GtkWidget *back = gtk_button_new_with_label("Back");
    //Make sure the widget does not span the entire window
    gtk_widget_set_halign(back, GTK_ALIGN_START);
    //Set up the "clicked" callback linking to the on_back_clicked function. 
    //Pass through the stack.
    g_signal_connect(back, "clicked", G_CALLBACK(on_back_clicked), stack);

    //Create a label for the name of the device, path to the event file, and ids of the device.
    //Ids include vendor and product number.
    GtkWidget *name_label = gtk_label_new("No device selected");
    GtkWidget *path_label = gtk_label_new("");
    GtkWidget *ids_label = gtk_label_new("");
    widgets->name_label = GTK_LABEL(name_label);
    widgets->path_label = GTK_LABEL(path_label);
    widgets->ids_label = GTK_LABEL(ids_label);

    //Add all of the previously created labels and buttons to the box
    gtk_box_append(GTK_BOX(box), back);
    gtk_box_append(GTK_BOX(box), name_label);
    gtk_box_append(GTK_BOX(box), path_label);
    gtk_box_append(GTK_BOX(box), ids_label);
    //Add separator which is a horizontal line across the window
    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    //Add the box to the outer box container
    gtk_box_append(GTK_BOX(outer), box);

    //Create a new box to hold the mouse sensitivity slider
    GtkWidget *sens_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(sens_box, 16);
    gtk_widget_set_margin_end(sens_box, 16);
    gtk_widget_set_margin_bottom(sens_box, 8);

    //Create a new static label
    GtkWidget *sens_label = gtk_label_new("Sensitivity:");
    //Make sure it is near the start (left side) of the box
    gtk_widget_set_halign(sens_label, GTK_ALIGN_START);

    //Create a new scale (slider), with a min range of 0.1 and max range of 5.0
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 5.0, 0.1);
    gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_RIGHT);
    //Set the default value of the scale to 1
    gtk_range_set_value(GTK_RANGE(scale), 1.0);
    //Allow the widget to expand and take up the space of the window
    gtk_widget_set_hexpand(scale, TRUE);
    //Set the editor widget for the sensitivity scale to the newly allocated scale
    widgets->sensitivity_scale = GTK_SCALE(scale);

    //Add the newly created elements to the sensitivity box
    gtk_box_append(GTK_BOX(sens_box), sens_label);
    gtk_box_append(GTK_BOX(sens_box), scale);
    //Add the sensitivity box to the outer container
    gtk_box_append(GTK_BOX(outer), sens_box);

    //Create a scrollable window to hold all of the programmable buttons
    GtkWidget *scroll = gtk_scrolled_window_new();
    //Allow it to expand all of the vertically
    gtk_widget_set_vexpand(scroll, TRUE);
    //Create a new box to hold the rows
    GtkWidget *rows_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(rows_box, 16);
    gtk_widget_set_margin_end(rows_box, 16);
    //Set the box for rows as a child of the scroll window
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), rows_box);
    //Add the scroll window to the outer container
    gtk_box_append(GTK_BOX(outer), scroll);
    //Set the editor widgets data
    widgets->buttons_rows_box = GTK_BOX(rows_box);

    //Create a new box for the bottom to allow for some empty space
    GtkWidget *bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(bottom, 8);
    gtk_widget_set_margin_bottom(bottom, 8);
    gtk_widget_set_margin_start(bottom, 16);
    gtk_widget_set_margin_end(bottom, 16);
    //Add the bottom box to the outer container
    gtk_box_append(GTK_BOX(outer), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    //Create a new button that allows the user to apply their changes
    GtkWidget *grab_btn = gtk_button_new_with_label("Grab Device");
    //Set the editor widgets data
    widgets->grab_btn = GTK_BUTTON(grab_btn);
    //Set up a callback for clicking the button. Link it to on_grab_clicked and pass the editor widgets.
    g_signal_connect(grab_btn, "clicked", G_CALLBACK(on_grab_clicked), widgets);
    //Add the button to the bottom box
    gtk_box_append(GTK_BOX(bottom), grab_btn);
    //Add the bottom box to the outer container
    gtk_box_append(GTK_BOX(outer), bottom);

    //Set the data for the outer box and link the editor widgets to it.
    //When the object is destroyed, have it link to the editor_widgets_free function.
    g_object_set_data_full(G_OBJECT(outer), "editor-widgets", widgets, editor_widgets_free);

    //Return the outer box, which holds all of the other elements.
    return outer;
}

void populate_editor(GtkStack *stack, MouseDevice_t *device)
{
    //Get the editor page child from the stack
    GtkWidget *editor = gtk_stack_get_child_by_name(stack, "editor");

    //Get the EditorWidgets_t struct that was linked to the editor page
    EditorWidgets_t *widgets = g_object_get_data(G_OBJECT(editor), "editor-widgets");

    //If it is null, return
    if (!widgets)
        return;

    //Set the labels of the name and path
    label_set_safe(widgets->name_label, device->name);
    label_set_safe(widgets->path_label, device->path);
    //gtk_label_set_text(widgets->name_label, device->name);
    //gtk_label_set_text(widgets->path_label, device->path);

    //Combien the product id and the vendor id in to one string
    char ids[64];
    snprintf(ids, sizeof(ids), "Vendor: %04x Product: %04x", device->vendor, device->product);
    label_set_safe(widgets->ids_label, ids);
    //gtk_label_set_text(widgets->ids_label, ids);

    //Set the current device to the device passed through
    widgets->current_device = *device;

    //Set the range value equal to the sensitivity of the device
    gtk_range_set_value(GTK_RANGE(widgets->sensitivity_scale), device->sensitivity);

    //Set the size of the remap row buttons to zero
    g_ptr_array_set_size(widgets->remap_rows, 0);

    //Remove the children from the button row box
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(GTK_WIDGET(widgets->buttons_rows_box))))
    {
        gtk_box_remove(widgets->buttons_rows_box, child);
    }

    //Loop through and create the correct amount of buttons for the device
    for (int i = 0; i < device->button_count; i++)
    {
        //Parent row
        RemapRow_t *row = NULL;
        //Create a row widget by calling build_remap_row
        GtkWidget *row_widget = build_remap_row(&widgets->current_device.buttons[i], &row);
        //Add it to the button row box
        gtk_box_append(widgets->buttons_rows_box, row_widget);
        //Add the row to the array of remap rows
        g_ptr_array_add(widgets->remap_rows, row);
    }

    //Set the editor page visible
    gtk_stack_set_visible_child_name(stack, "editor");
}