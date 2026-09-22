/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Miscellaneous macro definitions
 *  Copyright (C) 2018 Christopher Bazley
 */

#ifndef MISC_H
#define MISC_H

#include "MacroUtils.h"

/* Modify this definition for Unix or Windows file paths. */
#ifndef PATH_SEPARATOR
#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#elif defined(ACORN_C)
#define PATH_SEPARATOR '.'
#else
#define PATH_SEPARATOR '/'
#endif
#endif

#ifdef FORTIFY
#include "fortify.h"
#endif

#ifdef USE_CBDEBUG

#include "Debug.h"

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
#else
#define _Optional
#endif

#endif /* MISC_H */
