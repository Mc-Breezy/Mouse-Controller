#ifndef REMAP_H
#define REMAP_H

#include "input_detection.h"

/// @brief Attempts to apply the remaps of any keys that have been remapped. It also grabs the device so these changes apply.
/// @param device The MouseDevice_t struct of the device to grab.
/// @return Returns an int. 0 is a success while any other value is a failure.
int apply_remaps(MouseDevice_t *device);

/// @brief Disables the mouse remaps and ungrabs the device so any changes should not be active.
/// @param device The MouseDevice_t struct of the device to ungrab.
void disable_remaps(MouseDevice_t *device);

#endif