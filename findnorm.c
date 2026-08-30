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

/* ISO library header files */
#include <stdbool.h>

/* 3dObjLib headers */
#include "Vertex.h"
#include "Primitive.h"
#include "Vector.h"
#include "Coord.h"
#include "Group.h"

/* Local header files */
#include "findnorm.h"
#include "misc.h"

/* Source coordinates are integers, so an endpoint calculated from ideal
   geometry can be displaced by up to the square root of half a coordinate
   unit by rounding. */
static Coord const LineEndRounding = 0.707107;

static bool primitive_contains_shortened_line(
    Primitive * const polygon,
    Primitive const * const line,
    VertexArray const * const varray,
    Plane const plane)
{
  if (primitive_get_num_sides(line) != 2) {
    return false;
  }

  _Optional Coord (* const start)[3] = vertex_array_get_coords(
      varray, primitive_get_side(line, 0));
  _Optional Coord (* const end)[3] = vertex_array_get_coords(
      varray, primitive_get_side(line, 1));
  if (!start || !end) {
    return false;
  }

  Coord direction[3];
  vector_sub(&*end, &*start, &direction);
  Coord const length = vector_mag(&direction);
  if (length <= LineEndRounding * 2) {
    return false;
  }

  Coord adjustment[3];
  vector_mul(&direction, LineEndRounding / length, &adjustment);
  Coord short_start[3], short_end[3];
  vector_add(&*start, &adjustment, &short_start);
  vector_sub(&*end, &adjustment, &short_end);

  return primitive_contains_point(polygon, varray, &short_start, plane) &&
         primitive_contains_point(polygon, varray, &short_end, plane);
}

static _Optional Primitive *find_container_in_group(
                    const VertexArray *const varray, Primitive *const frontp,
                    const Group *const group, int back)
{
  _Optional Primitive *container = NULL;

  for (; (back >= 0) && (container == NULL); --back) {
    _Optional Primitive *const backp = group_get_primitive(group, back);
    if (!backp) {
      return NULL;
    }
    DEBUGF("Back primitive is %d (%p) in group %p\n", back, (void *)backp,
           (void *)group);

    /* Find the two-dimensional plane in which to check the two primitives
       for overlap (returns false if the back primitive is a point or line). */
    Plane plane;
    if (!primitive_find_plane(&*backp, varray, &plane)) {
      DEBUGF("Skipping %p: no plane\n", (void *)backp);
      continue;
    }

    /* Check that both primitives occupy the same plane */
    if (!primitive_coplanar(&*backp, frontp, varray)) {
      DEBUGF("Skipping %p: not coplanar\n", (void *)backp);
      continue;
    }

    /* Check that the front primitive is completely within the back polygon */
    if (primitive_contains(&*backp, frontp, varray, plane) ||
        primitive_contains_shortened_line(&*backp, frontp, varray, plane)) {
      container = backp;
      DEBUGF("Found container %p\n", (void *)container);
    }
  }
  return container;
}

static _Optional Primitive *find_container(VertexArray const * const varray,
                                           Group const * const groups,
                                           int const group)
{
  _Optional Primitive *container = NULL;
  assert(groups != NULL);
  assert(group >= 0);

  Group const * const front_group = groups + group;
  int const nprimitives = group_get_num_primitives(front_group);
  if (nprimitives > 0) {
    _Optional Primitive *const frontp = group_get_primitive(front_group, nprimitives-1);
    if (!frontp) {
      return NULL;
    }

    /* Search for a coplanar polygon drawn earlier in the same group
       which fully contains the most recently-added polygon. */
    if (nprimitives > 1) {
      DEBUGF("Searching same group %d (%p)\n", group, (void *)front_group);
      container = find_container_in_group(varray, &*frontp, front_group,
                                          nprimitives-2);
      DEBUGF("container %p\n", (void *)container);
    }

    /* Search for a containing polygon in previous groups. */
    for (int bg = 0; (bg < group) && (container == NULL); ++bg) {
      Group const * const back_group = groups + bg;
      DEBUGF("Searching previous group %d (%p)\n", bg, (void *)back_group);

      const int back = group_get_num_primitives(back_group)-1;
      container = find_container_in_group(varray, &*frontp, back_group, back);
      DEBUGF("container %p\n", (void *)container);
    }
  }
  return container;
}

bool find_container_normal(VertexArray const * const varray,
                           Group const * const groups, int const group,
                           Coord (*const normal)[3])
{
  bool got_normal = false;
  _Optional Primitive *const container = find_container(varray, groups, group);
  if (container != NULL) {
    got_normal = primitive_get_normal(&*container, varray, normal);
  }
  return got_normal;
}
