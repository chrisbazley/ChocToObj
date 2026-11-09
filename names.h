/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Chocks Away object names
 *  Copyright (C) 2018 Christopher Bazley
 */

#ifndef CHOCNAMES_H
#define CHOCNAMES_H

#include "strb.h"

/* Write the object name at the current position; return EOF on failure, zero on success. */
int get_obj_name(strb_t *name, int index);
int get_obj_name_extra(strb_t *name, int index);

#endif /* CHOCNAMES_H */
