#include "remap.h"
#include <libevdev/libevdev.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>

//Global variables that are used throughout the functions of this source file
  //src_dev is a libevedev created for the current device that is being grabbed
static struct libevdev *src_dev = NULL;
  //src_fd is a file descriptor for the event file of the device being grabbed
static int src_fd = -1;
  //uinput_fd is a file descriptor for the virtual device created.
  //Inputs are written to this file manually.
static int uinput_fd = -1;
  //The thread that runs to check for input changes to apply
static GThread *remap_thread = NULL;
  //Boolean for if the thread is running
static volatile gboolean running = FALSE;

  //Accumulation on the x axis of input to carry over
static float accum_x = 0.0f;
  //Accumulation on the y axis of input to carry over
static float accum_y = 0.0f;

/// @brief Writes an event to the file descriptor given
/// @param fd The file descriptor of the file to write the event to.
/// @param type The evdev event type of the event.
/// @param code The evdev event code of the event.
/// @param value The evdev value of the event.
static void write_event(int fd, int type, int code, int value)
{
    //Declare a blank input_event struct
    struct input_event ev;
    //Zero out the data
    memset(&ev, 0, sizeof(ev));
    //Set the type equal to the type passed into the function
    ev.type = type;
    //Set the code equal to the code passed into the function
    ev.code = code;
    //Set the value equal to the value passed into the function
    ev.value = value;
    //Write the event to the file descriptor passed into the function
    write(fd, &ev, sizeof(ev));
}

/// @brief The main loop ran by the GThread that checks for input and overwrites any keys that are remapped.
/// @param user_data The user data passed to the function (MouseDevice_t)
/// @return Returns null
static gpointer remap_loop(gpointer user_data)
{
    //Cast the user_data to a MouseDevice_t struct
    MouseDevice_t *device = user_data;
    //Declare a blank input_event
    struct input_event ev;

    //While the thread is running
    while (running)
    {
        //Read the next event from the src_dev struct with a normal flag and a blocking flag
        int rc = libevdev_next_event(src_dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);

        //If the event is still reading a sync status, continue
        if (rc == LIBEVDEV_READ_STATUS_SYNC) continue;
        //If the event is not read succesfully, break from the loop
        if (rc != LIBEVDEV_READ_STATUS_SUCCESS) break;

        //Check if the event is a scan or msc event, if so, block it to prevent any possibility of it using the original input
        if (ev.type == EV_MSC && ev.code == MSC_SCAN)
            continue;

        //Check if the type of the event is a EV_KEY
        if (ev.type == EV_KEY)
        {
            //For each button in the device, check if it is a code that is equal to the current event code.
            for (int i = 0; i < device->button_count; i++)
            {
                MouseButtons_t *button = &device->buttons[i];

                if (button->code == ev.code)
                {
                    //If the code matches an existing button, check if the button is enabled.
                    if (!button->enabled) goto skip;
                    //If it is enabled, set the code of the event equal to the remapped value.
                    ev.code = button->remap_code;
                    //Break from the loop.
                    break;
                }
            }
        }

        //Check if the type is EV_REL and if the code is either REL_X or REL_Y.
        //This is for changing the mouse sensitivity via a multiplier
        if (ev.type == EV_REL && (ev.code == REL_X || ev.code == REL_Y))
        {
            //Change the x
            if (ev.code == REL_X)
            {
                //Since some data is lost with a conversion from a float to an int, store the value in an external variable
                accum_x += ev.value * device->sensitivity;
                //Set the output equal to the product of the value and sensitivity casted to an int
                int out = (int)accum_x;
                //Subtract out from accum_x to correct the offset and accumulated value
                accum_x -= out;
                //If out is zero, continue as no changes need to be made
                if (out == 0) continue;
                //Set the value of the event equal to the out variable
                ev.value = out;
            }
            else if (ev.code == REL_Y) //Change the y
            {
                //Same explanation as the REL_X change but for the Y axis
                accum_y += ev.value * device->sensitivity;
                int out = (int)accum_y;
                accum_y -= out;
                if (out == 0) continue;
                ev.value = out;
            }
        }

        //Write the event to the virtual device event file manually
        write_event(uinput_fd, ev.type, ev.code, ev.value);
        //Write a sync event to make sure the system flushes the input and uses the event
        write_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
      
        skip:;
    }

    return NULL;
}

void disable_remaps(MouseDevice_t *device)
{
    //The thread should not be running
    running = FALSE;
    //Zero out the accumulation of the x and the y input
    accum_x = 0.0f;
    accum_y = 0.0f;
    
    //If the thread was running and defined, dereference it with g_thread_join and set it to null
    if (remap_thread)
    {
        g_thread_join(remap_thread);
        remap_thread = NULL;
    }

    //If the libevdev struct was allocated, free it and set it equal to null
    if (src_dev) { libevdev_free(src_dev); src_dev = NULL; }
    //If the src_fd is greater than zero, close the file and set it to a negative integer
    if (src_fd > 0) { close(src_fd); src_fd = -1; }

    //If the virtual device input file descriptor is greater than or equal to zero
    if (uinput_fd >= 0)
    {
        //Destroy the virtual device
        ioctl(uinput_fd, UI_DEV_DESTROY);
        //Close the file
        close(uinput_fd);
        //Set the file descriptor to a negative number
        uinput_fd = -1;
    }
}

int apply_remaps(MouseDevice_t *device)
{
    //Make sure the everything is cleaned up properly before applying any changes
    disable_remaps(device);

    //Open the original event file for the device passed through
    src_fd = open(device->path, O_RDWR);
    //If the file descriptor is less than zero, there was an error opening the file. Print error and return a failure
    if (src_fd < 0) { perror("Failed to open event file"); return -1; }

    //Create a new libevdev from the file descriptor src_fd
    if (libevdev_new_from_fd(src_fd, &src_dev) < 0)
    {
        //If it fails to create, print an error, close the event file, and return a failure
        perror("Failed to create libevdev device");
        close(src_fd);
        src_fd = -1;
        return -1;
    }

    //Attempt to grab the device and be able to change and overwrite input
    if (libevdev_grab(src_dev, LIBEVDEV_GRAB) < 0)
    {
        //If it fails to grab, print an error and disable_remaps to clean up
        perror("Failed to grab device");
        disable_remaps(device);
        //Return a failure
        return -1;
    }

    //Open up the virtual device input file as write only
    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    //If the file descriptor is less than zero, there was an error opening the file.
    //Disable any remaps to cleanup everything. Return a failure
    if (uinput_fd < 0) { perror("Failed to open /dev/uinput"); disable_remaps(device); return -1; }

    //Set the virtual device to be able to transmit key related events, relative axis events, and sync events.
    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN);

    //Loop through different key codes on the keyboard and add them to the virtual device
    for (int i = KEY_ESC; i <= KEY_MICMUTE; i++)
        ioctl(uinput_fd, UI_SET_KEYBIT, i);
    
    //Loop through buttons on the mouse and add them to the virtual device
    for (int i = BTN_LEFT; i <= BTN_TASK; i++)
        ioctl(uinput_fd, UI_SET_KEYBIT, i);
    
    //Add the keys for relative x, y, and the scroll wheel
    ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_HWHEEL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_HWHEEL_HI_RES);

    //Declare a uinput_setup struct
    struct uinput_setup usetup;
    //Zero it out
    memset(&usetup, 0, sizeof(usetup));
    //Set the bustype to be usb
    usetup.id.bustype = BUS_USB;
    //Set the vendor and product id to whatever since it is a virtual device
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    //Set the name of the virtual device
    strncpy(usetup.name, "Mouse Controller Virtual Device", UINPUT_MAX_NAME_SIZE);

    //Attempt to setup the virtual device properly
    if (ioctl(uinput_fd, UI_DEV_SETUP, &usetup) < 0)
    {
        //If it fails, print an error and close the event file
        perror("UI_DEV_SETUP failed");
        close(uinput_fd);
        uinput_fd = -1;
        //Cleanup before returning a failure
        disable_remaps(device);
        return -1;
    }

    //Attempt to create the device after setting it up
    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0)
    {
        //If it fails, print an error and close the event file
        perror("UI_DEV_CREATE failed");
        close(uinput_fd);
        uinput_fd = -1;
        //Cleanup before returning a failure
        disable_remaps(device);
        return -1;
    }

    //Set up the thread to run and connect it to the remap_loop function with the device data passed through
    running = TRUE;
    remap_thread = g_thread_new("remap", remap_loop, device);

    //Return zero which is a success
    return 0;
}
