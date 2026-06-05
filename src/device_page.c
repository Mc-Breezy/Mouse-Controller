#include "device_page.h"
#include "editor_page.h"

/// @brief Callback for when a device button is clicked on the device page
/// @param button The button that was clicked
/// @param user_data The data passed through the callback (ButtonData_t)
static void on_device_clicked(GtkButton *button, gpointer user_data)
{
    //Retrieve the ButtonData_t from the user_data
    ButtonData_t *data = user_data;

    //Populate the editor page with the info of the device
    populate_editor(data->stack, &data->device);
}

/// @brief Creates a button that displays the correlated MouseDevice info.
/// @param info Information of the device.
/// @param stack The main stack of the program.
/// @return Returns the button created with the label of the mouse device name and an image.
static GtkWidget *make_device_button(MouseDevice_t *info, GtkStack *stack)
{
    //Allocate a new button data struct
    ButtonData_t *data = g_new(ButtonData_t, 1);
    //Set the device info equal to the info passed through
    data->device = *info;
    //Set the stack equal to the stack passed through
    //Both of these will be saved for later
    data->stack = stack; 

    //Button that the user will be allowed to click to go to the editor page for the device
    GtkWidget *button = gtk_button_new();
    //Prevent the button from expanding more than it needs to
    gtk_widget_set_halign(button, GTK_ALIGN_START);
    gtk_widget_set_valign(button, GTK_ALIGN_START);
    //Prevent it from expanding horizontally
    gtk_widget_set_hexpand(button, FALSE);

    //Create a callback for the "clicked" event and link it to on_device_clicked and pass the button data
    g_signal_connect(button, "clicked", G_CALLBACK(on_device_clicked), data);
    //Set the button data connected to the button to make sure the data is freed
    g_object_set_data_full(G_OBJECT(button), "button-data", data, g_free);

    //Inner box that will hold an image and a label
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);

    //Creates an image that links to an icon of a mouse
    GtkWidget *image = gtk_image_new_from_icon_name("input-mouse");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 48);

    //Creates a label for the device name
    GtkWidget *label = gtk_label_new(info->name);
    //Allow the label to wrap if necessary
    gtk_label_set_wrap(GTK_LABEL(label), TRUE);
    //Set the max amount of characters to 24
    gtk_label_set_max_width_chars(GTK_LABEL(label), 24);
    //Set the halign of the button to the center
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);

    //Add the image to the box
    gtk_box_append(GTK_BOX(box), image);
    //Add the label to the box
    gtk_box_append(GTK_BOX(box), label);
    //Set the box as a child of the button
    gtk_button_set_child(GTK_BUTTON(button), box);

    //Return the device button that was created
    return button;
}

GtkWidget *build_device_page(MouseDevice_t *mice, int count, GtkStack *stack)
{
    //Create a scrollable window for the chance there is more devices than can fit on the window
    GtkWidget *scroll = gtk_scrolled_window_new();
    //Set it so that the scrollable window can scroll vertically but not horizontally
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    //Create a flow box so elements naturally wrap around
    GtkWidget *flow = gtk_flow_box_new();
    //Set the minimum amount of children per line
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow), 2);
    //Set the maximum amount of children per line
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow), 6);
    //Set the row spacing
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flow), 12);
    //Set the column spacing
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flow), 12);
    //Prevent the entries from taking up the entire space of the cell
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flow), FALSE);
    //Disable the highlight selection of the cell
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow), GTK_SELECTION_NONE);
    //Set the margins of the top, bottom, start, and end
    gtk_widget_set_margin_top(flow, 16);
    gtk_widget_set_margin_bottom(flow, 16);
    gtk_widget_set_margin_start(flow, 16);
    gtk_widget_set_margin_end(flow, 16);
    //Set the halign and valign
    gtk_widget_set_halign(flow, GTK_ALIGN_START);
    gtk_widget_set_valign(flow, GTK_ALIGN_START);

    //Check if the amount of mouse devices is zero
    if (count == 0)
    {
        //Display a message saying there are no mouse devices connected
        GtkWidget *msg = gtk_label_new("No mouse devices found");
        gtk_flow_box_append(GTK_FLOW_BOX(flow), msg);
    }
    else
    {
        //For each mouse device, create a button and add it to the flow box
        for (int i = 0; i < count; i++)
        {
            GtkWidget *button = make_device_button(&mice[i], stack);
            gtk_flow_box_append(GTK_FLOW_BOX(flow), button);
        }
    }

    //Set the flow box as a child of the scrollable window
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), flow);

    //Return the scrollable window, which is the widget that is the device page
    return scroll;
}