#include "remap_row.h"
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include "utility.h"

const char *keycode_name(int code)
{
    //Check for well known buttons
    if (code == BTN_LEFT) return "Left Click";
    if (code == BTN_RIGHT) return "Right Click";
    if (code == BTN_MIDDLE) return "Middle Click";
    if (code == BTN_SIDE) return "Side Button";
    if (code == BTN_EXTRA) return "Extra Button";
    if (code == BTN_FORWARD) return "Forward";
    if (code == BTN_BACK) return "Back";
    if (code == BTN_TASK) return "Task";

    //If it is not a well known button, try and get the code name from evdev
    const char *name = libevdev_event_code_get_name(EV_KEY, code);
    //Return the name from evdev, or "Unknown" if evdev could not get the name
    return name ? name : "Unknown";
}

/// @brief Callback for when the listen button is clicked in a row
/// @param btn The button that was clicked
/// @param user_data The user data passed to the function (RemapRow_t)
static void on_listen_clicked(GtkButton *btn, gpointer user_data)
{
    //Cast the user_data to RemapRow_t
    RemapRow_t *row = user_data;
    //Invert the listening boolean
    row->listening = !row->listening;

    //If the row is listening, set the label text and add styling
    if (row->listening)
    {
        gtk_button_set_label(btn, "Waiting...");
        gtk_widget_add_css_class(GTK_WIDGET(btn), "listening");
    }
    else //Else, set the label back to default and remove the styling
    {
        gtk_button_set_label(btn, "Listen");
        gtk_widget_remove_css_class(GTK_WIDGET(btn), "listening");
    }
}

/// @brief Callback for when the reset button is clicked in a row
/// @param btn The button that was clicked
/// @param user_data The user data passed to the function through the callback (RemapRow_t)
static void on_reset_clicked(GtkButton *btn, gpointer user_data)
{
    //Cast the user_data to RemapRow_t
    RemapRow_t *row = user_data;
    //Set the remap_code equal to the original button code
    row->button->remap_code = row->button->code;
    //Set the label text
    gtk_label_set_text(row->binding_label, keycode_name(row->button->code));

    //If the row is listening, reset the state so it is not.
    //Also change the label and remove the styling.
    if (row->listening)
    {
        row->listening = FALSE;
        gtk_button_set_label(row->listen_btn, "Listen");
        gtk_widget_remove_css_class(GTK_WIDGET(row->listen_btn), "listening");
    }
}

/// @brief Callback for when the enable check box is toggled
/// @param check The check box that was toggled
/// @param user_data User data passed to the function through the callback (RemapRow_t)
static void on_enable_toggled(GtkCheckButton *check, gpointer user_data)
{
    //Cast the user_data to RemapRow_t
    RemapRow_t *row = user_data;
    //Set the enabled data field for the button equal to the value of the check box
    row->button->enabled = gtk_check_button_get_active(check);
}

GtkWidget *build_remap_row(MouseButtons_t *button, RemapRow_t **out_row)
{
    //Allocate a new RemapRow_t struct and zero out the fields
    RemapRow_t *row = g_new0(RemapRow_t, 1);
    //Set the button field equal to the button data passed to the function
    row->button = button;
    //Set listening to false
    row->listening = FALSE;

    //Create a box with horizontal orientation to hold the GUI elements
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(box, 6);
    gtk_widget_set_margin_bottom(box, 6);

    //Create a name label for the name of the button 
    GtkWidget *name_label = gtk_label_new(button->name);
    gtk_widget_set_size_request(name_label, 140, -1);
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    //Add it to the box
    gtk_box_append(GTK_BOX(box), name_label);

    //Left over from debugging, can be simplified.
    //Makes a utf8 safe string from the current binding name
    const char *binding_str = keycode_name(button->remap_code);
    gchar *safe = g_utf8_make_valid(binding_str ? binding_str : "Unknown", -1);
    //Create the label for the current binding key name
    GtkWidget *binding_label = gtk_label_new(safe);
    //Free the allocated string 
    g_free(safe);
    gtk_widget_set_size_request(binding_label, 140, -1);
    gtk_widget_set_halign(binding_label, GTK_ALIGN_START);
    //Set the row binding_label data
    row->binding_label = GTK_LABEL(binding_label);
    //Add the label to the box
    gtk_box_append(GTK_BOX(box), binding_label);

    //Create a button to allow the program to listen for input
    GtkWidget *listen_btn = gtk_button_new_with_label("Listen");
    //Set the listen_btn in the row struct
    row->listen_btn = GTK_BUTTON(listen_btn);
    //Create a callback to the on_listen_clicked function and pass the RemapRow_t data
    g_signal_connect(listen_btn, "clicked", G_CALLBACK(on_listen_clicked), row);
    //Add it to the box
    gtk_box_append(GTK_BOX(box), listen_btn);

    //Create a reset button
    GtkWidget *reset_btn = gtk_button_new_with_label("Reset");
    //Create a callback that links the "clicked" event to the function on_reset_clicked passing the row data
    g_signal_connect(reset_btn, "clicked", G_CALLBACK(on_reset_clicked), row);
    gtk_box_append(GTK_BOX(box), reset_btn);

    //Create a checkbox button
    GtkWidget *toggle = gtk_check_button_new_with_label("Enabled");
    //Set the default state to true
    gtk_check_button_set_active(GTK_CHECK_BUTTON(toggle), button->enabled);
    //Create a callback linking the "toggled" event to on_enable_toggled and passing the row data
    g_signal_connect(toggle, "toggled", G_CALLBACK(on_enable_toggled), row);
    //Add it to the box
    gtk_box_append(GTK_BOX(box), toggle);

    //Connect the RemapRow_t data to the box 
    g_object_set_data_full(G_OBJECT(box), "remap-row", row, g_free);

    //If there is an out_row pointer passed, set it equal to row
    if (out_row) *out_row = row;
    //Return the box which holds all of the GUI elements for the row
    return box;
}