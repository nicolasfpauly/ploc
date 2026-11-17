/* Shared sound/screen parameter laws. GPL-3.0-or-later. */
#ifndef PLOC_PARAMS_H
#define PLOC_PARAMS_H
#include <stdint.h>
#define PLOC_CURRENT_MODEL 3
/* Six bins, 0..5 extra droplets. The main bubble is always present. */
static inline int ploc_drops(int word) { return ((uint32_t)word*6u)>>15; }
#endif
