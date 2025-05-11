/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Miscellaneous macro definitions
 *  Copyright (C) 2018 Christopher Bazley
 */

#ifndef MISC_H
#define MISC_H

#define PI (3.1415926535897896)

/* Modify this definition for Unix or Windows file paths. */
#define PATH_SEPARATOR '.'

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/* Suppress compiler warnings about an unused function argument. */
#define NOT_USED(x) ((void)(x))

#ifdef FORTIFY
#include "Fortify.h"
#endif

#ifdef USE_CBDEBUG

#include "Debug.h"
#include "PseudoIO.h"

#else /* USE_CBDEBUG */

#include <stdio.h>
#include <assert.h>

#define DEBUG_SET_OUTPUT(output_mode, log_name)

#ifdef DEBUG_OUTPUT
#define DEBUGF if (1) printf
#else
#define DEBUGF if (0) printf
#endif /* DEBUG_OUTPUT */

#endif /* USE_CBDEBUG */

#ifdef USE_OPTIONAL
#include "Optional.h"
#endif

#define STRING_OR_NULL(s) ((s) == NULL ? "" : &*(s))

#endif /* MISC_H */
