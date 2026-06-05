#include "input_detection.h"
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <glib.h>

static const struct { int code; const char *name; } KNOWN_BUTTONS[] = 
{
    { BTN_LEFT, "Left Button" },
    { BTN_RIGHT, "Right Button" },
    { BTN_MIDDLE, "Middle Button" },
    { BTN_SIDE, "Side Button" },
    { BTN_EXTRA, "Extra Button" },
    { BTN_FORWARD, "Forward" },
    { BTN_BACK, "Back" },
    { BTN_TASK, "Task" }
};

static const int N_KNOWN = sizeof(KNOWN_BUTTONS) / sizeof(KNOWN_BUTTONS[0]);

void scan_mice(void)
{
    //Open up the directory with the input devices events
    DIR *directory = opendir("/dev/input");

    //If the directory fails to open, print an error and return
    if (!directory)
    {
        perror("Failed to open even directory");
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(directory)) != NULL)
    {
        //Check if the name of the file is an event
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;

        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        struct libevdev *dev = NULL;

        if (libevdev_new_from_fd(fd, &dev) < 0)
        {
            close(fd);
            continue;
        }

        if (libevdev_has_event_type(dev, EV_REL) &&
            libevdev_has_event_code(dev, EV_REL, REL_X) &&
            libevdev_has_event_code(dev, EV_REL, REL_Y))
            {
                printf("Found mouse: %s\n", path);
                printf("  Name: %s\n", libevdev_get_name(dev));
                printf("  ID: bus=%04x vendor=%04x product=04x\n",
                libevdev_get_id_bustype(dev), 
                libevdev_get_id_vendor(dev),
                libevdev_get_id_product(dev));
            }
        
        libevdev_free(dev);
        close(fd);
    }

    closedir(directory);
}

MouseDevice_t *get_mice(int *count)
{
    //Set count to zero in case user forgot to not initialize a default value
    *count = 0;

    //Capacity of the current array of structs
    int capacity = 8;
    
    //The array of mouse devices
    MouseDevice_t *devices = malloc(sizeof(MouseDevice_t) * capacity);

    //Check if allocation failed
    if (!devices)
        return NULL;

    //Open the /dev/input directory where the event files are stored
    DIR *dir = opendir("/dev/input");

    //If the directory fails to open, print an error and return NULL
    if (!dir)
    {
        perror("Failed to open the events directory");
        free(devices);
        return NULL;
    }

    //Struct for a directory entry
    struct dirent *entry;

    //Scan through all of the files of the directory
    while ((entry = readdir(dir)) != NULL)
    {
        //Check if the beginning of the file name is equal to "event"
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;

        //Create the file path to the event file
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

        //Get a file descriptor to the event file, this will be used later to construct a libevdev struct
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        
        //If the file descriptor is less than zero, the file failed to open, so continue
        if (fd < 0)
            continue;

        struct libevdev *dev = NULL;
        
        //If it fails to allocate a new libevdev struct, close the file and continue. 
        if (libevdev_new_from_fd(fd, &dev) < 0)
        {
            close(fd);
            continue;
        }

        //Check if the libevdev device has the attributes of a mouse
        if (libevdev_has_event_type(dev, EV_REL) && libevdev_has_event_code(dev, EV_REL, REL_X) && libevdev_has_event_code(dev, EV_REL, REL_Y))
        {
            //Check if the size or count of the array is greater than the capacity of the array
            if (*count >= capacity)
            {
                //Increase capacity
                capacity *= 2;

                //Reallocate the array
                MouseDevice_t *tmp = realloc(devices, sizeof(MouseDevice_t) * capacity);

                //If reallocation fails, free everything and return null
                if (!tmp)
                {
                    free(devices);
                    libevdev_free(dev);
                    close(fd);
                    closedir(dir);
                    return NULL;
                }

                //Set the array of devices equal to the new tmp array
                devices = tmp;
            }

            //Get the entry at the count (size) of the array
            MouseDevice_t *device_info = &devices[*count];

            //Copy over the path into the path of the MouseDevice info struct
            strncpy(device_info->path, path, sizeof(device_info->path) - 1);
            
            //Copy over the name into the name variable of the MouseDevice info struct
            strncpy(device_info->name, libevdev_get_name(dev), sizeof(device_info->name) - 1);

            if (!g_utf8_validate(device_info->name, -1, NULL))
            {
                gchar *clean = g_utf8_make_valid(device_info->name, -1);
                strncpy(device_info->name, clean, sizeof(device_info->name) - 1);
                g_free(clean);
            }

            //Store the vendor, product, and bustype of the device
            device_info->vendor = libevdev_get_id_vendor(dev);
            device_info->product = libevdev_get_id_product(dev);
            device_info->bustype = libevdev_get_id_bustype(dev);

            //Detect how many buttons the mouse has, and what buttons
            int btn_count = 0;
            for (int i = 0; i < N_KNOWN; i++)
            {
                //See if the device has the event codes for the known buttons on a mouse
                if (libevdev_has_event_code(dev, EV_KEY, KNOWN_BUTTONS[i].code))
                {
                    //Store that information
                    MouseButtons_t *buttons = &device_info->buttons[btn_count++];
                    buttons->code = KNOWN_BUTTONS[i].code;
                    buttons->remap_code = KNOWN_BUTTONS[i].code;
                    buttons->enabled = 1;
                    strncpy(buttons->name, KNOWN_BUTTONS[i].name, sizeof(buttons->name) - 1);
                }
            }

            device_info->button_count = btn_count;
            device_info->sensitivity = 1.0f;

            //Increment count
            (*count)++;
        }

        libevdev_free(dev);
        close(fd);
    }

    closedir(dir);
    return devices;
}

void free_mice(MouseDevice_t *mice)
{
    free(mice);
}
