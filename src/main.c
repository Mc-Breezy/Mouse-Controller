#include "device_page.h"
#include "editor_page.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#define DEFAULT_WINDOW_HEIGHT 400
#define DEFAULT_WINDOW_WIDTH 600

/// @brief Finishes the gtk alert dialog prompt if the user is not running the application as root.
static void on_error_response(GObject *dialog, GAsyncResult *res, gpointer *app)
{
    gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(dialog), res, NULL);
    g_application_quit(G_APPLICATION(app));
}

/// @brief The main callback function used when the gtk app is created.
static void activate(GtkApplication *app, gpointer user_data)
{
    //Window variable declaration
	GtkWidget *window;

    //Create a new window and set the default title and size
	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Mouse Controller");
	gtk_window_set_default_size(GTK_WINDOW(window), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

    //Check if the application is ran as super user, if not, show alert dialog and exit
    if (getuid() != 0)
    {
        //Create a new gtk alert dialog with the header "Permission Denied"
        GtkAlertDialog *dialog = gtk_alert_dialog_new("Permission Denied");
        //Set the fine detail of the dialog box
        gtk_alert_dialog_set_detail(dialog, "Application must be ran as root!\n");
        //Have one button on the alert dialog that says "OK", when clicked, it will close the prompt
        gtk_alert_dialog_set_buttons(dialog, (const char *[]){"OK", NULL});
        //Link the dialog button event to the on_error_response functino that will finish the dialog and close the application
        gtk_alert_dialog_choose(dialog, GTK_WINDOW(window), NULL, on_error_response, app);
        //Unref the dialog
        g_object_unref(dialog);

        //Prevent any other code from running
        return;
    }

    //Create a stack that allows multiple pages to be toggled on or off
    GtkWidget *stack = gtk_stack_new();
    gtk_window_set_child(GTK_WINDOW(window), stack);

    //Build the editor page and add it to the stack of pages
    GtkWidget *editor_page = build_editor_page(GTK_STACK(stack), GTK_WINDOW(window));
    gtk_stack_add_named(GTK_STACK(stack), editor_page, "editor");

    //Get all of the mouse devices for some reason this can include keyboards
    int count = 0;
    MouseDevice_t *mice = get_mice(&count);
    //Construct the device page so the user can select the device they want to edit
    GtkWidget *device_page = build_device_page(mice, count, GTK_STACK(stack));
    //Add it to the stack of pages
    gtk_stack_add_named(GTK_STACK(stack), device_page, "device-list");
    //Free the array of mice
    free_mice(mice);

    //Show the device list as default
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "device-list");

    //Present the window
	gtk_window_present(GTK_WINDOW(window));

}

/// @brief The main function or entry point for the program.
/// @return Returns the status of running the gtk application.
int main(int argc, char **argv)
{
    //GtkApplication pointer and status variable declaration
    GtkApplication *app;
	int status;

    //Create new GtkApplication
	app = gtk_application_new("com.breeze.mousecontroller", G_APPLICATION_DEFAULT_FLAGS);

    //Connect the activate callback
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    //Run the application
	status = g_application_run(G_APPLICATION(app), argc, argv);
    
    //Cleanup
	g_object_unref(app);

	return status;
}
