#ifndef REMAP_H
#define REMAP_H

#include "input_detection.h"

/// @brief 
/// @param device 
/// @return 
int apply_remaps(MouseDevice_t *device);

/// @brief 
/// @param device 
void disable_remaps(MouseDevice_t *device);

#endif