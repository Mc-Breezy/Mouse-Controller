#include "utility.h"
#include <glib.h>

/// @brief A function to check if the string is utf8 valid, if it is not, it converts it to a valid string.
/// @param label Label to set the text of.
/// @param str Text to test for utf8 validity. 
void label_set_safe(GtkLabel *label, const char *str)
{
    //Check if the string passed is null or empty
    if (!str || str[0] == '\0')
    {
        gtk_label_set_text(label, "Unknown");
        return;
    }

    //If the string is valid, set the label
    if (g_utf8_validate(str, -1, NULL))
    {
        gtk_label_set_text(label, str);
    }
    else
    {
        //Make a clean string
        gchar *clean = g_utf8_make_valid(str, -1);
        //Set the label test
        gtk_label_set_text(label, clean);
        //Free the string
        g_free(clean);
    }
}