/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Chocks Away object mesh parser
 *  Copyright (C) 2018 Christopher Bazley
 */

#ifndef PARSER_H
#define PARSER_H

/* ISO C library headers */
#include <stdio.h>
#include <stdbool.h>

/* StreamLib headers */
#include "Reader.h"

bool choc_to_obj(Reader *index, Reader *models, FILE *out,
                 const int first, const int last, const char *name,
                 const long int data_start, const char *mtl_file,
                 double const thick, const unsigned int flags);

#endif /* PARSER_H */
