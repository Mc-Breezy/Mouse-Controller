#ifndef INPUT_DETECTION_H
#define INPUT_DETECTION_H

#define MAX_BUTTONS 64

/*
 * The MouseButtons_t struct contains information about the key code of a mouse button,
 * and what that button is remapped to. Also whether it is enabled or disabled and its name.
 */
typedef struct
{
    int code;
    char name[64];
    int remap_code;
    int enabled;
} MouseButtons_t;

/*
 * The MouseDevice_t struct holds relevant information relating to a mouse device.
 * It stores the path to the event file, the name of the mouse, the product ID, vendor ID, and bustype.
 * The struct also stores the buttons it has, the button_count, and sensitivity.
 */
typedef struct
{
    char path[64];
    char name[256];
    unsigned int product;
    unsigned int vendor;
    unsigned int bustype;
    MouseButtons_t buttons[MAX_BUTTONS];
    int button_count;
    float sensitivity;
} MouseDevice_t;

/// @brief Debug function to print all available mouse inputs. 
void scan_mice(void);

/// @brief Function opens up /dev/input and reads the event files and returns any inputs that could be a mouse.
/// @param count A pointer to an int that is used to keep track of the size of the returned array. 
/// @return Returns a pointer to an array of MouseDevice_t.
MouseDevice_t *get_mice(int *count);

/// @brief Frees the list of mice.
/// @param mice A pointer to an array of MouseDevice_t structs.
void free_mice(MouseDevice_t *mice);

#endif