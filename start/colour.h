#ifndef COLOUR_H
#define COLOUR_H

#include <libpynq.h>

void colour_init(void);
const char* get_colour(void);
void colour_destroy(void);

#endif // COLOUR_H