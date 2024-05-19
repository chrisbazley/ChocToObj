/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Find the normal vector of a containing polygon
 *  Copyright (C) 2018 Christopher Bazley
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public Licence as published by
 *  the Free Software Foundation; either version 2 of the Licence, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public Licence for more details.
 *
 *  You should have received a copy of the GNU General Public Licence
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef FINDNORM_H
#define FINDNORM_H

/* ISO C library headers */
#include <stdbool.h>

/* 3dObjLib headers */
#include "Vertex.h"
#include "Coord.h"
#include "Group.h"

bool find_container_normal(VertexArray const *varray,
                           Group const *groups, int group,
                           Coord (*normal)[3]);

#endif /* FINDNORM_H */
