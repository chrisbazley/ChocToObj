/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Chocks Away object mesh parser
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
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>

/* StreamLib headers */
#include "Reader.h"

/* 3dObjLib headers */
#include "Vector.h"
#include "Coord.h"
#include "Vertex.h"
#include "Primitive.h"
#include "Group.h"
#include "Clip.h"
#include "ObjFile.h"

/* Local header files */
#include "flags.h"
#include "parser.h"
#include "version.h"
#include "names.h"
#include "findnorm.h"
#include "colours.h"
#include "misc.h"

/* Unless we do something about, all of the objects appear reflected in the
   Z axis. */
#define FLIP_Z (1)

enum {
  MaxNumPrimitives = 255,
  MaxNumVertices = 200,
  MinNumSides = 2,
  MaxNumSides = 8,
  BytesPerPrimitive = 16,
  PaddingBeforePrimSimpDist = 3,
  BytesPerVertex = 12,
  PaddingBeforeClipDist = 4,
  WhiteColour = 0xff,
  OrangeColour = 0x56,
  BlackColour = 0x0,
  PeridotColour = 0x74,
  PeruColour = 0x5c,
  DarkGreyColour = 3,
  NColours = 256,
  NTints = 1 << 2,
};

/* Special numbers for the third vertex */
enum {
  Special8DashThinWhiteLine = 0xfd,
  Special16DashThinWhiteLine = 0xfe,
  Special32DashThickWhiteLine = 0xff
};

enum {
 Special32OrangePoints = 0xf8,
 Special16DarkGreyQuads = 0xf9,
 Special64ThickPeruLines = 0xfa,
 Special16ThinBlackZigZags = 0xfb,
 Special8PeridotQuadsCheckZ = 0xfc,
 Special16WhiteQuadsCheckZ = 0xfd,
 Special8PeridotQuads = 0xfe,
 Special16WhiteQuads = 0xff
};

/* Primitive plot styles */
enum {
  Outline_None = 0,
  Outline_Black = 1,
  Outline_Blue = 2
};

enum {
  Group_Simple,
  Group_Complex,
  Group_Count
};

static bool parse_vertices(Reader * const r, const int object_count,
                          VertexArray * const varray,
                          const int nvertices, const int nsvertices,
                          const unsigned int flags)
{
  assert(r != NULL);
  assert(!reader_ferror(r));
  assert(object_count >= 0);
  assert(varray != NULL);
  assert(nvertices > 0);
  assert(nvertices <= MaxNumVertices);
  assert(nsvertices > 0);
  assert(nsvertices <= nvertices);
  assert(!(flags & ~FLAGS_ALL));

  if (flags & FLAGS_VERBOSE) {
    long int const pos = reader_ftell(r);
    printf("Found %d (%d) vertices at file position %ld (0x%lx)\n",
           nvertices, nsvertices, pos, pos);
  }

  int n;
  if (flags & FLAGS_LIST) {
    n = 0;
  } else {
    n = (flags & FLAGS_SIMPLE) ? nsvertices : nvertices;
    if (vertex_array_alloc_vertices(varray, n) < n) {
      fprintf(stderr, "Failed to allocate memory for %d vertices "
              "(object %d)\n", n, object_count);
      return false;
    }
  }

  for (int v = 0; v < n; ++v) {
    Coord pos[3] = {0.0, 0.0, 0.0};
    for (size_t dim = 0; dim < ARRAY_SIZE(pos); ++dim) {
      int32_t coord;
      if (!reader_fread_int32(&coord, r)) {
        fprintf(stderr, "Failed to read vertex %d\n", v);
        return false;
      }
      pos[dim] = coord;
    } /* next dimension */

#if FLIP_Z
    pos[2] = -pos[2]; /* flip z axis */
#endif
    if (vertex_array_add_vertex(varray, &pos) < 0) {
      fprintf(stderr, "Failed to allocate vertex memory "
              "(vertex %d of object %d)\n", v, object_count);
      return false;
    }

    if (flags & FLAGS_VERBOSE) {
      vertex_array_print_vertex(varray, v);
      puts("");
    }
  } /* next vertex */

  /* Skip the (remaining) vertex data */
  if (reader_fseek(r, BytesPerVertex * ((long)nvertices - n), SEEK_CUR)) {
    fprintf(stderr, "Failed to seek end of vertices (object %d)\n",
            object_count);
    return false;
  }

  return true;

}

static int add_special_vertex(VertexArray * const varray,
                              Coord (* const coords)[3])
{
  int const v = vertex_array_add_vertex(varray, coords);
  if (v < 0) {
    fprintf(stderr, "Failed to allocate vertex memory for "
            "procedural geometry\n");
  }
  return v;
}

static _Optional Primitive *add_special_primitive(Group * const group)
{
  _Optional Primitive *const primitive = group_add_primitive(group);
  if (primitive == NULL) {
    fprintf(stderr, "Failed to allocate primitive memory for "
            "procedural geometry\n");
  }
  return primitive;
}

static int add_special_side(Primitive * const primitive, const int v)
{
  int const s = primitive_add_side(primitive, v);
  if (s < 0) {
    fprintf(stderr, "Failed to add side for procedural geometry: "
            "too many sides?\n");
  }
  return s;
}


static bool make_special_zigzags(VertexArray * const varray,
                                 Group (* const groups)[Group_Count],
                                 int const group, int const n,
                                 int const colour, unsigned int const flags)
{
  assert(groups != NULL);
  assert(group >= 0);
  assert(group < Group_Count);
  assert(n >= 1);
  assert(colour >= 0);
  assert(colour < NColours);
  assert(!(flags & ~FLAGS_ALL));

  int const p = group_get_num_primitives((*groups) + group);
  _Optional Primitive *const pp = group_get_primitive((*groups) + group, p-1);
  if (!pp) {
    return false;
  }
  assert(primitive_get_num_sides(&*pp) == 3);

  const int vw = primitive_get_side(&*pp, 0);
  const int vs = primitive_get_side(&*pp, 1);
  const int ve = primitive_get_side(&*pp, 2);
  const int id = primitive_get_id(&*pp);

  _Optional Coord (*cw)[3] = vertex_array_get_coords(varray, vw),
                  (*cs)[3] = vertex_array_get_coords(varray, vs),
                  (*ce)[3] = vertex_array_get_coords(varray, ve);
  if (!cw || !cs || !ce) {
    return false;
  }

  Coord vecl[3], vecw[3];
  vector_sub(&*ce, &*cs, &vecl);
  vector_sub(&*cw, &*cs, &vecw);

  int vlast = vs;

  primitive_delete_all(&*pp);

  for (int d = 0; d < n; ++d) {
    _Optional Primitive *seg = NULL;
    if (d == 0) {
      seg = pp;
    } else {
      seg = add_special_primitive((*groups) + group);
      if (seg == NULL) {
        return false;
      }
      primitive_set_id(&*seg, id);
    }

    Coord coords[3];
    vector_mul(&vecl, (Coord)(int)(d+1)/n, &coords);
    if ((d % 2) == 0) {
      vector_add(&coords, &vecw, &coords);
    }
    vector_add(&*cs, &coords, &coords);

    const int v = add_special_vertex(varray, &coords);
    if (v < 0) {
      return false;
    }

    primitive_set_colour(&*seg, colour); /* Use the hard-wired colour */

    if ((add_special_side(&*seg, vlast) < 0) ||
        (add_special_side(&*seg, v) < 0)) {
      return false;
    }
    vlast = v;

    if (flags & FLAGS_VERBOSE) {
      printf("Special %s; primitive %d in group %d:\n", (d % 2) ? "zag" : "zig",
             group_get_num_primitives((*groups) + group)-1, group);
      primitive_print(&*seg, varray);
      puts("");
    }
  }

  return true;
}

static bool get_thick_vec(Coord (*const normal)[3],
                          Coord (*const vecl)[3],
                          Coord const thick,
                          Coord (*const vecw)[3])
{
  assert(thick >= 0);
  assert(normal != NULL);
  assert(vecw != NULL);
  assert(vecl != NULL);

  /* Compute a vector perpendicular to the most-recently-defined polygon
     in the same plane as the polygon containing it. */
  bool got_thick = false;
  Coord cross_prod[3];
  vector_cross(normal, vecl, &cross_prod);
  if (vector_norm(&cross_prod, vecw)) {
    vector_mul(vecw, thick, vecw);
    got_thick = true;
    DEBUGF("Found thickening vector\n");
  }
  return got_thick;
}

static bool make_special_hatch(VertexArray * const varray,
                               Group (* const groups)[Group_Count],
                               int const group, int const n,
                               int const colour, Coord const thick,
                               unsigned int const flags)
{
  assert(groups != NULL);
  assert(group >= 0);
  assert(group < Group_Count);
  assert(n >= 1);
  assert(colour >= 0);
  assert(colour < NColours);
  assert(thick >= 0);
  assert(!(flags & ~FLAGS_ALL));

  int const p = group_get_num_primitives((*groups) + group);
  _Optional Primitive *const pp = group_get_primitive((*groups) + group, p-1);
  if (!pp) {
    return false;
  }
  assert(primitive_get_num_sides(&*pp) == 3);

  const int vw = primitive_get_side(&*pp, 0);
  const int vs = primitive_get_side(&*pp, 1);
  const int ve = primitive_get_side(&*pp, 2);
  const int id = primitive_get_id(&*pp);

  _Optional Coord (*cw)[3] = vertex_array_get_coords(varray, vw),
                  (*cs)[3] = vertex_array_get_coords(varray, vs),
                  (*ce)[3] = vertex_array_get_coords(varray, ve);
  if (!cw || !cs || !ce) {
    return false;
  }

  Coord vecl[3], vecw[3];
  vector_sub(&*ce, &*cs, &vecl);
  vector_sub(&*cw, &*cs, &vecw);

  Coord thickvec[3], negthickvec[3], norm[3], negvecw[3];
  bool thicken = false, reverse = false;
  if (thick == 0) {
    DEBUGF("Thickening disabled\n");
  } else if (find_container_normal(varray, *groups, group, &norm)) {
    thicken = get_thick_vec(&norm, &vecw, thick/2, &thickvec);
    if (thicken) {
      if (flags & FLAGS_VERBOSE) {
        printf("Thickening hatched parallelogram %d in group %d\n",
               p, group);
      }
      vector_mul(&thickvec, -2, &negthickvec);
      vector_mul(&vecw, -1, &negvecw);
    }
  }

  primitive_delete_all(&*pp);

  /* There's a fencepost error in the game where it only draws 64 of
     the 65 railway sleepers needed to complete the pattern; the error
     shifts from one end to the other depending on which is nearer. */
  for (int d = 0; d < n; ++d) {
    _Optional Primitive *seg = NULL;
    if (d == 0) {
      seg = pp;
    } else {
      seg = add_special_primitive((*groups) + group);
      if (seg == NULL) {
        return false;
      }
      primitive_set_id(&*seg, id);
    }

    int v[4];
    int num_sides = 0;
    Coord coords[3];
    vector_mul(&vecl, (Coord)d/n, &coords);
    vector_add(&*cs, &coords, &coords);

    if (thicken) {
      vector_add(&coords, &thickvec, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }

      vector_add(&coords, &vecw, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }

      vector_add(&coords, &negthickvec, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }

      vector_add(&coords, &negvecw, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }
    } else {
      if (d == 0) {
        v[num_sides++] = vs;
      } else {
        v[num_sides] = add_special_vertex(varray, &coords);
        if (v[num_sides++] < 0) {
          return false;
        }
      }

      vector_add(&coords, &vecw, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }
    }

    for (int s = 0; s < num_sides; ++s) {
      int const t = reverse ? (num_sides - 1 - s) : s;
      if (add_special_side(&*seg, v[t]) < 0) {
        return false;
      }
    }

    /* Check that the first quad has the same normal vector (i.e. faces the
       same direction) as its container. If not, reverse the direction of all
       future quads. */
    if ((d == 0) && thicken) {
      reverse = primitive_set_normal(&*seg, varray, &norm);
    }

    primitive_set_colour(&*seg, colour); /* Use the hard-wired colour */

    if (flags & FLAGS_VERBOSE) {
      printf("Special parallel; primitive %d in group %d:\n",
             group_get_num_primitives((*groups) + group)-1, group);
      primitive_print(&*seg, varray);
      puts("");
    }
  }

  return true;
}

static bool make_special_quads(VertexArray * const varray,
                               Group (* const groups)[Group_Count],
                               int const group, int const n,
                               int const colour, unsigned int const flags)
{
  assert(groups != NULL);
  assert(group >= 0);
  assert(group < Group_Count);
  assert(n >= 1);
  assert(colour >= 0);
  assert(colour < NColours);
  assert(!(flags & ~FLAGS_ALL));

  int const p = group_get_num_primitives((*groups) + group);
  Primitive *const pp = group_get_primitive((*groups) + group, p-1);
  if (!pp) {
    return false;
  }
  assert(primitive_get_num_sides(pp) == 3);

  const int vs = primitive_get_side(pp, 0);
  const int ve = primitive_get_side(pp, 1);
  const int vw = primitive_get_side(pp, 2);
  const int id = primitive_get_id(pp);

  _Optional Coord (*cw)[3] = vertex_array_get_coords(varray, vw),
                  (*cs)[3] = vertex_array_get_coords(varray, vs),
                  (*ce)[3] = vertex_array_get_coords(varray, ve);
  if (!cw || !cs || !ce) {
    return false;
  }

  Coord vecl[3], vecw[3];
  vector_sub(&*ce, &*cs, &vecl);
  vector_sub(&*cw, &*cs, &vecw);


  Coord norm[3];
  bool reverse = false;
  bool got_normal = find_container_normal(varray, *groups, group, &norm);
  if (!got_normal) {
    /* Try to find a container facing the opposite direction */
    primitive_reverse_sides(pp);
    got_normal = find_container_normal(varray, *groups, group, &norm);
    primitive_reverse_sides(pp);
  }

  Coord quadl[3];
  vector_mul(&vecl, 1/((Coord)n * 2), &quadl);

  primitive_delete_all(pp);

  for (int d = 0; d < n; ++d) {
    int num_sides = 0;
    int v[4];
    Coord quad_start[3];
    vector_mul(&vecl, (Coord)d/n, &quad_start);
    vector_add(&*cs, &quad_start, &quad_start);

    _Optional Primitive *quad = NULL, *back_quad = NULL;

    if (d == 0) {
      quad = pp;
      v[num_sides++] = vs;
    } else {
      quad = add_special_primitive((*groups) + group);
      if (quad == NULL) {
        return false;
      }
      primitive_set_id(&*quad, id);

      v[num_sides] = add_special_vertex(varray, &quad_start);
      if (v[num_sides++] < 0) {
        return false;
      }
    }

    Coord quad_end[3];
    vector_add(&quad_start, &quadl, &quad_end);
    v[num_sides] = add_special_vertex(varray, &quad_end);
    if (v[num_sides++] < 0) {
      return false;
    }

    vector_add(&quad_end, &vecw, &quad_end);
    v[num_sides] = add_special_vertex(varray, &quad_end);
    if (v[num_sides++] < 0) {
      return false;
    }

    if (d == 0) {
      v[num_sides++] = vw;
    } else {
      vector_add(&quad_start, &vecw, &quad_start);
      v[num_sides] = add_special_vertex(varray, &quad_start);
      if (v[num_sides++] < 0) {
        return false;
      }
    }

    for (int s = 0; s < num_sides; ++s) {
      int const t = reverse ? (num_sides - 1 - s) : s;
      if (add_special_side(&*quad, v[t]) < 0) {
        return false;
      }
    }

    /* Check that the first quad has the same normal vector (i.e. faces the
       same direction) as its container. If not, reverse the direction of all
       future quads. */
    if ((d == 0) && got_normal) {
      reverse = primitive_set_normal(&*quad, varray, &norm);
    }

    primitive_set_colour(&*quad, colour); /* Use the hard-wired colour */

    if (flags & FLAGS_VERBOSE) {
      printf("Special %sparallelogram; primitive %d in group %d:\n",
             got_normal ? "" : "front ",
             group_get_num_primitives((*groups) + group)-1, group);
      primitive_print(&*quad, varray);
      puts("");
    }

    if (got_normal) {
      continue;
    }

    /* The game doesn't cull backfacing special polygons and in principle
       there is no way to tell which way they should face since they aren't
       necessarily coplanar with any other polygon, so add a back side. */
    back_quad = add_special_primitive((*groups) + group);
    if (back_quad == NULL) {
      return false;
    }
    primitive_set_id(&*back_quad, id);
    primitive_set_colour(&*back_quad, colour);

    assert(!reverse);
    for (int s = 0; s < num_sides; ++s) {
      int const t = num_sides - 1 - s;
      if (add_special_side(&*back_quad, v[t]) < 0) {
        return false;
      }
    }

    if (flags & FLAGS_VERBOSE) {
      printf("Special back parallelogram; primitive %d in group %d:\n",
             group_get_num_primitives((*groups) + group)-1, group);
      primitive_print(&*back_quad, varray);
      puts("");
    }
  }

  return true;
}

static bool make_special_points(VertexArray * const varray,
                                Group (* const groups)[Group_Count],
                                int const group, int const n,
                                int const colour, unsigned int const flags)
{
  assert(groups != NULL);
  assert(group >= 0);
  assert(group < Group_Count);
  assert(n >= 1);
  assert(colour >= 0);
  assert(colour < NColours);
  assert(!(flags & ~FLAGS_ALL));

  int const p = group_get_num_primitives((*groups) + group);
  _Optional Primitive *const pp = group_get_primitive((*groups) + group, p-1);
  if (!pp) {
    return false;
  }
  assert(primitive_get_num_sides(&*pp) == 3);

  const int vs = primitive_get_side(&*pp, 0);
  const int ve = primitive_get_side(&*pp, 1);
  /* The third vertex is ignored */
  const int id = primitive_get_id(&*pp);

  _Optional Coord (*cs)[3] = vertex_array_get_coords(varray, vs),
                  (*ce)[3] = vertex_array_get_coords(varray, ve);
  if (!cs || !ce) {
    return false;
  }

  Coord vec[3];
  vector_sub(&*ce, &*cs, &vec);

  Coord twicen = (int)(n * 2);

  primitive_delete_all(&*pp);

  for (int d = 0; d < n; ++d) {
    Coord coords[3];
    vector_mul(&vec, (int)((d * 2) + 1)/twicen, &coords);
    vector_add(&*cs, &coords, &coords);

    _Optional Primitive *point = NULL;
    if (d == 0) {
      point = pp;
    } else {
      point = add_special_primitive((*groups) + group);
      if (point == NULL) {
        return false;
      }
      primitive_set_id(&*point, id);
    }

    primitive_set_colour(&*point, colour); /* Use the hard-wired colour */

    const int v = add_special_vertex(varray, &coords);
    if (v < 0) {
      return false;
    }
    if (add_special_side(&*point, v) < 0) {
      return false;
    }

    if (flags & FLAGS_VERBOSE) {
      printf("Special point; primitive %d in group %d:\n",
             group_get_num_primitives((*groups) + group)-1, group);
      primitive_print(&*point, varray);
      puts("");
    }
  }

  return true;
}

static bool make_special_dashed(VertexArray * const varray,
                                Group (* const groups)[Group_Count],
                                int const group, int const n,
                                int const colour, Coord const thick,
                                unsigned int const flags)
{
  assert(groups != NULL);
  assert(group >= 0);
  assert(group < Group_Count);
  assert(n >= 1);
  assert(colour >= 0);
  assert(colour < NColours);
  assert(thick >= 0);
  assert(!(flags & ~FLAGS_ALL));

  int const p = group_get_num_primitives((*groups) + group);
  _Optional Primitive *const pp = group_get_primitive((*groups) + group, p-1);
  if (!pp) {
    return false;
  }
  assert(primitive_get_num_sides(&*pp) == 2);

  const int vs = primitive_get_side(&*pp, 0);
  const int ve = primitive_get_side(&*pp, 1);
  const int id = primitive_get_id(&*pp);

  _Optional Coord (*cs)[3] = vertex_array_get_coords(varray, vs),
                  (*ce)[3] = vertex_array_get_coords(varray, ve);
  if (!cs || !ce) {
    return false;
  }

  Coord vec[3];
  vector_sub(&*ce, &*cs, &vec);

  Coord dashl[3];
  vector_mul(&vec, 1/((Coord)n * 2), &dashl);

  Coord thickvec[3], negthickvec[3], norm[3], negdashl[3];
  bool thicken = false, reverse = false;
  if (thick == 0) {
    DEBUGF("Thickening disabled\n");
  } else if (find_container_normal(varray, *groups, group, &norm)) {
    thicken = get_thick_vec(&norm, &vec, thick/2, &thickvec);
    if (thicken) {
      if (flags & FLAGS_VERBOSE) {
        printf("Thickening dashed line %d in group %d\n", p, group);
      }
      vector_mul(&thickvec, -2, &negthickvec);
      vector_mul(&dashl, -1, &negdashl);
    }
  }

  primitive_delete_all(&*pp);

  for (int d = 0; d < n; ++d) {
    int v[4];
    int num_sides = 0;
    Coord coords[3];
    vector_mul(&vec, (Coord)d/n, &coords);
    vector_add(&*cs, &coords, &coords);

    _Optional Primitive *dash = NULL;
    if (d == 0) {
      dash = pp;
    } else {
      dash = add_special_primitive((*groups) + group);
      if (dash == NULL) {
        return false;
      }
      primitive_set_id(&*dash, id);
    }

    if (thicken) {
      vector_add(&coords, &thickvec, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }

      vector_add(&coords, &dashl, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }

      vector_add(&coords, &negthickvec, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }

      vector_add(&coords, &negdashl, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }
    } else {
      if (d == 0) {
        v[num_sides++] = vs;
      } else {
        v[num_sides] = add_special_vertex(varray, &coords);
        if (v[num_sides++] < 0) {
          return false;
        }
      }

      vector_add(&coords, &dashl, &coords);
      v[num_sides] = add_special_vertex(varray, &coords);
      if (v[num_sides++] < 0) {
        return false;
      }
    }

    for (int s = 0; s < num_sides; ++s) {
      int const t = reverse ? (num_sides - 1 - s) : s;
      if (add_special_side(&*dash, v[t]) < 0) {
        return false;
      }
    }

    /* Check that the first quad has the same normal vector (i.e. faces the
       same direction) as its container. If not, reverse the direction of all
       future quads. */
    if ((d == 0) && thicken) {
      reverse = primitive_set_normal(&*dash, varray, &norm);
    }

    primitive_set_colour(&*dash, colour); /* Use the hard-wired colour */

    if (flags & FLAGS_VERBOSE) {
      printf("Special dash; primitive %d in group %d:\n",
             group_get_num_primitives((*groups) + group)-1, group);
      primitive_print(&*dash, varray);
      puts("");
    }
  }

  return true;
}

static bool thicken_line(VertexArray * const varray,
                         Group (* const groups)[Group_Count],
                         int const group, Coord const thick,
                         unsigned int const flags)
{
  assert(groups != NULL);
  assert(group >= 0);
  assert(group < Group_Count);
  assert(thick > 0);
  assert(!(flags & ~FLAGS_ALL));

  int const p = group_get_num_primitives((*groups) + group);
  _Optional Primitive *const pp = group_get_primitive((*groups) + group, p-1);
  if (!pp) {
    return false;
  }
  assert(primitive_get_num_sides(&*pp) == 2);

  const int vs = primitive_get_side(&*pp, 0);
  const int ve = primitive_get_side(&*pp, 1);

  _Optional Coord (*cs)[3] = vertex_array_get_coords(varray, vs),
                  (*ce)[3] = vertex_array_get_coords(varray, ve);
  if (!cs || !ce) {
    return false;
  }

  Coord vec[3];
  vector_sub(&*ce, &*cs, &vec);

  Coord thickvec[3], norm[3];
  bool thicken = false;

  if (find_container_normal(varray, *groups, group, &norm)) {
    thicken = get_thick_vec(&norm, &vec, thick/2, &thickvec);
  }

  if (thicken) {
    if (flags & FLAGS_VERBOSE) {
      printf("Thickening line %d in group %d\n",
             group_get_num_primitives((*groups) + group), group);
    }

    Coord coords[3], negthickvec[3], negvec[3];
    int v[4];
    int num_sides = 0;
    vector_mul(&thickvec, -2, &negthickvec);
    vector_mul(&vec, -1, &negvec);

    vector_add(&*cs, &thickvec, &coords);
    v[num_sides] = add_special_vertex(varray, &coords);
    if (v[num_sides++] < 0) {
      return false;
    }

    vector_add(&coords, &vec, &coords);
    v[num_sides] = add_special_vertex(varray, &coords);
    if (v[num_sides++] < 0) {
      return false;
    }

    vector_add(&coords, &negthickvec, &coords);
    v[num_sides] = add_special_vertex(varray, &coords);
    if (v[num_sides++] < 0) {
      return false;
    }

    vector_add(&coords, &negvec, &coords);
    v[num_sides] = add_special_vertex(varray, &coords);
    if (v[num_sides++] < 0) {
      return false;
    }

    primitive_delete_all(&*pp);

    for (int s = 0; s < num_sides; ++s) {
      if (add_special_side(&*pp, v[s]) < 0) {
        return false;
      }
    }

    /* Ensure that the quad has the same normal vector (i.e. faces the
       same direction) as its container. */
    (void)primitive_set_normal(&*pp, varray, &norm);
  }

  return true;
}

static void flip_backfacing(VertexArray * const varray,
                            Group (* const groups)[Group_Count],
                            const unsigned int flags)
{
  assert(varray != NULL);
  assert(groups != NULL);
  assert(flags & FLAGS_FLIP_BACKFACING);
  assert(!(flags & ~FLAGS_ALL));

  Coord norm[3] = {0,0,1};
  for (int g = 0; g < Group_Count; ++g) {
    int const n = group_get_num_primitives((*groups) + g);
    for (int p = 0; p < n; ++p) {
      _Optional Primitive *const pp = group_get_primitive((*groups) + g, p);
      if (!pp) {
        continue;
      }
      if (primitive_set_normal(&*pp, varray, &norm)) {
        if (flags & FLAGS_VERBOSE) {
          printf("Flipped ground polygon %d in group %d\n", p, g);
        }
      }
    }
  }
}

static bool parse_primitives(Reader * const r, const int object_count,
                             VertexArray * const varray,
                             Group (* const groups)[Group_Count],
                             const int32_t simple_dist,
                             const int nprimitives, const int nsprimitives,
                             Coord const thick, const unsigned int flags)
{
  assert(r != NULL);
  assert(object_count >= 0);
  assert(!reader_ferror(r));
  assert(groups != NULL);
  assert(simple_dist >= 0);
  assert(nprimitives > 0);
  assert(nsprimitives > 0);
  assert(nsprimitives <= nprimitives);
  assert(thick >= 0);
  assert(!(flags & ~FLAGS_ALL));

  if (flags & FLAGS_VERBOSE) {
    long int const pos = reader_ftell(r);
    printf("Found %d (%d) primitives at file position %ld (0x%lx)\n",
           nprimitives, nsprimitives, pos, pos);
  }

  int n;
  if (flags & FLAGS_LIST) {
    n = 0;
  } else {
    n = (flags & FLAGS_SIMPLE) ? nsprimitives : nprimitives;
  }

  bool all_z_0 = (flags & FLAGS_FLIP_BACKFACING) != 0;

  for (int p = 0; p < n; ++p) {
    const int group = p < nsprimitives ? Group_Simple : Group_Complex;
    const long int primitive_start = reader_ftell(r);
    if (flags & FLAGS_VERBOSE) {
      printf("Found sides in group %d at file position %ld (0x%lx)\n",
             group, primitive_start, primitive_start);
    }

    _Optional Primitive * const pp = group_add_primitive((*groups) + group);
    if (pp == NULL) {
      fprintf(stderr, "Failed to allocate primitive memory "
              "(primitive %d of object %d)\n", p, object_count);
      return false;
    }
    primitive_set_id(&*pp, group_get_num_primitives((*groups) + group));

    /* We need to read the primitive definition into a temporary array so
       that we can get its simplification distance before validating the
       vertex indices. */
    int sides[MaxNumSides];
    int nsides;
    for (nsides = 0; nsides < MaxNumSides; ++nsides) {
      sides[nsides] = reader_fgetc(r);
      if (sides[nsides] == EOF) {
        fprintf(stderr, "Failed to read side %d of primitive %d "
                "of object %d\n", nsides, p, object_count);
        return false;
      }

      if (sides[nsides] == 0) {
        break;
      }
    }

    /* Skip the unused vertex indices */
    if (reader_fseek(r, primitive_start + MaxNumSides, SEEK_SET)) {
      fprintf(stderr, "Failed to seek end of primitive "
              "(primitive %d of object %d)\n", p, object_count);
      return false;
    }

    const int colour = reader_fgetc(r);
    if (colour == EOF) {
      fprintf(stderr, "Failed to read colour (primitive %d of object %d)\n",
              p, object_count);
      return false;
    }
    primitive_set_colour(&*pp, colour);

    if (reader_fseek(r, PaddingBeforePrimSimpDist, SEEK_CUR)) {
      fprintf(stderr, "Failed to seek polygon simplification distance "
              "(primitive %d of object %d)\n", p, object_count);
      return false;
    }

    int32_t prim_simple_dist;
    if (!reader_fread_int32(&prim_simple_dist, r)) {
      fprintf(stderr, "Failed to read polygon simplification distance "
              "(primitive %d of object %d)\n", p, object_count);
      return false;
    }

    if (prim_simple_dist < 0) {
      fprintf(stderr, "Bad polygon simplification distance, %" PRId32 " "
              "(primitive %d of object %d)\n", prim_simple_dist, p,
              object_count);
      return false;
    }

    /* If the polygon's simplification distance is not greater than the
       model's then it must be simplified whenever the model is. This
       inference prevents vertices not included in the simplified model's
       vertex count from being reported as errors. */
    if ((flags & FLAGS_SIMPLE) && (prim_simple_dist <= simple_dist) &&
        (nsides > 2)) {
      nsides = 2;
      if (flags & FLAGS_VERBOSE) {
        printf("Simplifying primitive %d in group %d "
               "(distance %" PRId32 " <= %" PRId32 ")\n", p, group,
               prim_simple_dist, simple_dist);
      }
    }

    const int nvertices = vertex_array_get_num_vertices(varray);
    bool special = false;
    for (int s = 0; s < nsides; ++s) {
      int v = sides[s];
      switch (s) {
      case 2: /* Check for special lines */
        switch (v) {
        case Special8DashThinWhiteLine:
          special = true;
          if (!make_special_dashed(varray, groups, group, 8,
                                   WhiteColour, thick, flags)) {
            fprintf(stderr, "Failed to make a thin dashed line "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        case Special16DashThinWhiteLine:
          special = true;
          if (!make_special_dashed(varray, groups, group, 16,
                                   WhiteColour, thick, flags)) {
            fprintf(stderr, "Failed to make a thin dashed line "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        case Special32DashThickWhiteLine:
          special = true;
          if (!make_special_dashed(varray, groups, group, 32,
                                   WhiteColour, thick*2, flags)) {
            fprintf(stderr, "Failed to make a thick dashed line "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        default:
          break; /* ordinary line */
        }
        break;

      case 3: /* Check for special triangles */
        switch (v) {
        case Special32OrangePoints:
          special = true;
          if (!make_special_points(varray, groups, group, 32,
                                   OrangeColour, flags)) {
            fprintf(stderr, "Failed to make a dotted line "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        case Special16DarkGreyQuads:
          special = true;
          if (!make_special_quads(varray, groups, group, 16,
                                  DarkGreyColour, flags)) {
            fprintf(stderr, "Failed to make a row of parallelograms "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        case Special64ThickPeruLines:
          special = true;
          if (!make_special_hatch(varray, groups, group, 64,
                                  PeruColour, thick*2, flags)) {
            fprintf(stderr, "Failed to make a hatched region "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        case Special16ThinBlackZigZags:
          special = true;
          if (!make_special_zigzags(varray, groups, group, 16,
                                    BlackColour, flags)) {
            fprintf(stderr, "Failed to make a zigzag line "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        case Special8PeridotQuadsCheckZ:
        case Special8PeridotQuads:
          special = true;
          if (!make_special_quads(varray, groups, group, 8,
                                  PeridotColour, flags)) {
            fprintf(stderr, "Failed to make a row of parallelograms "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        case Special16WhiteQuadsCheckZ:
        case Special16WhiteQuads:
          special = true;
          if (!make_special_quads(varray, groups, group, 16,
                                  WhiteColour, flags)) {
            fprintf(stderr, "Failed to make a row of parallelograms "
                    "(primitive %d of object %d)\n", p, object_count);
            return false;
          }
          break;

        default:
          break; /* ordinary triangle */
        }
        break;

      default:
        break;
      }

      if (special) {
        break;
      }

      /* Validate the vertex indices */
      if (v < 1 || v > nvertices) {
        fprintf(stderr, "Bad vertex %lld (side %d of primitive %d "
                "of object %d)\n", (long long signed)v - 1, s, p,
                object_count);
        return false;
      }

      /* Vertex indices are stored using offset-1 encoding */
      --v;

      if (all_z_0) {
        _Optional Coord (* const coords)[3] = vertex_array_get_coords(varray, v);
        if (!coords) {
          return false;
        }
        if (!coord_equal((*coords)[2], 0)) {
          DEBUGF("Not a flat object (vertex %d, z==%g)\n", v, (*coords)[2]);
          all_z_0 = false;
        }
      }

      if (primitive_add_side(&*pp, v) < 0) {
        fprintf(stderr, "Failed to add side: too many sides? "
                        "(side %d of primitive %d of object %d)\n",
                s, p, object_count);
        return false;
      }
    }

    if (!special) {
#if FLIP_Z
      /* Inverting the Z coordinate axis makes all primitives back-facing
         unless we also reverse the order in which their vertices are
         specified. */
      primitive_reverse_sides(&*pp);
#endif

      const int num_sides = primitive_get_num_sides(&*pp);
      if (num_sides < MinNumSides) {
        fprintf(stderr, "Bad side count %d (primitive %d of object %d)\n",
                num_sides, p, object_count);
        return false;
      }

      int const side = primitive_get_skew_side(&*pp, varray);
      if (side >= 0) {
        fprintf(stderr, "Warning: skew polygon detected "
                        "(side %d of primitive %d of object %d)\n",
                side, p, object_count);
      }

      if ((num_sides == 2) && (thick > 0)) {
        /* Thicken a line if it is coplanar with a polygon. */
        if (!thicken_line(varray, groups, group, thick, flags)) {
          fprintf(stderr, "Failed to thicken a line "
                          "(primitive %d of object %d)\n", p, object_count);
          return false;
        }
      }

      if (flags & FLAGS_VERBOSE) {
        printf("Primitive %d in group %d:\n",
               group_get_num_primitives((*groups) + group) - 1, group);
        primitive_print(&*pp, varray);
        puts("");
      }
    }
  } /* next primitive */

  /* Skip the (remaining) primitive data */
  if (reader_fseek(r, BytesPerPrimitive * ((long)nprimitives - n),
                   SEEK_CUR)) {
    fprintf(stderr, "Failed to seek end of primitives (object %d)\n",
            object_count);
    return false;
  }

  /* Fix up polygons which appear to facing the wrong way (i.e. those
     belonging to objects coplanar with z=0). The game disables backface
     culling for arbitrary objects so we have to use an heuristic instead. */
  if (all_z_0) {
    flip_backfacing(varray, groups, flags);
  }
  return true;
}

static void mark_vertices(VertexArray * const varray,
                          Group (* const groups)[Group_Count],
                          const int object_count,
                          const unsigned int flags)
{
  assert(groups != NULL);
  assert(object_count >= 0);
  assert(!(flags & ~FLAGS_ALL));

  if (flags & FLAGS_UNUSED) {
    /* We're keeping all vertices */
    vertex_array_set_all_used(varray);
  } else {
    /* Mark only the used vertices */
    for (int g = 0; g < Group_Count; ++g) {
      group_set_used((*groups) + g, varray);
    }

    /* Report the unused vertices */
    if (flags & FLAGS_VERBOSE) {
      int count = 0;
      const int nvertices = vertex_array_get_num_vertices(varray);
      for (int v = 0; v < nvertices; ++v) {
        if (!vertex_array_is_used(varray, v)) {
          _Optional Coord (*coords)[3] = vertex_array_get_coords(varray, v);
          if (!coords) {
            continue;
          }
          printf("Vertex %d {%g,%g,%g} is unused (object %d)\n", v,
                 (*coords)[0], (*coords)[1], (*coords)[2], object_count);
          ++count;
        }
      }
      printf("Object %d has %d unused vertices\n", object_count, count);
    }
  }
}

static char const *style_to_string(int32_t pstyle)
{
  char const *s;

  switch (pstyle) {
  case Outline_Black:
    s = "Black polygon outlines, thick lines";
    break;

  case Outline_Blue:
    s = "Blue polygon outlines, thick lines";
    break;

  default:
    assert(pstyle == Outline_None);
    s = "No polygon outlines, thin lines";
    break;
  }
  return s;
}

static int get_false_colour(const Primitive *pp, _Optional void *arg)
{
  NOT_USED(pp);
  NOT_USED(arg);

  static int p;
  const int colour = (p * NTints) % NColours;
  ++p;
  return colour;
}

static int get_human_material(char *buf, size_t buf_size,
                              int const colour, _Optional void *arg)
{
  NOT_USED(arg);
  return snprintf(buf, buf_size, "%s_%d",
                  get_colour_name(colour / NTints), colour % NTints);
}

static int get_material(char *const buf, size_t const buf_size,
                        int const colour, _Optional void *arg)
{
  NOT_USED(arg);
  return snprintf(buf, buf_size, "riscos_%d", colour);
}

static bool process_object(Reader * const r, FILE * const out,
                           const char * const object_name,
                           const int object_count,
                           VertexArray * const varray,
                           Group (* const groups)[Group_Count],
                           int *const vtotal, bool *const list_title,
                           Coord const thick, long int const data_start,
                           const unsigned int flags)
{
  long int obj_start = 0;

  assert(r != NULL);
  assert(!reader_ferror(r));
  assert(object_name != NULL);
  assert(*object_name != '\0');
  assert(object_count >= 0);
  assert(groups != NULL);
  assert(vtotal != NULL);
  assert(*vtotal >= 0);
  assert(thick >= 0);
  assert(data_start >= 0);
  assert(!(flags & ~FLAGS_ALL));

  if (flags & FLAGS_LIST) {
    obj_start = reader_ftell(r);
  }

  int32_t simple_dist;
  if (!reader_fread_int32(&simple_dist, r)) {
    fprintf(stderr, "Failed to read simplification distance (object %d)\n",
            object_count);
    return false;
  }
  if (simple_dist < 0) {
    fprintf(stderr, "Bad simplification distance, %" PRId32 " (object %d)\n",
            simple_dist, object_count);
    return false;
  }

  int32_t nprimitives;
  if (!reader_fread_int32(&nprimitives, r)) {
    fprintf(stderr, "Failed to read number of primitives (object %d)\n",
            object_count);
    return false;
  }

  if (nprimitives >= MaxNumPrimitives) {
    fprintf(stderr, "Bad number of primitives, %lld (object %d)\n",
            (long long signed int)nprimitives + 1, object_count);
    return false;
  }
  ++nprimitives;

  int32_t nvertices;
  if (!reader_fread_int32(&nvertices, r)) {
    fprintf(stderr, "Failed to read number of vertices (object %d)\n",
            object_count);
    return false;
  }
  if (nvertices < 0 || nvertices >= MaxNumVertices) {
    fprintf(stderr, "Bad number of vertices, %lld (object %d)\n",
            (long long signed int)nvertices + 1, object_count);
    return false;
  }
  ++nvertices;

  int32_t nsprimitives;
  if (!reader_fread_int32(&nsprimitives, r)) {
    fprintf(stderr, "Failed to read simplified number of primitives "
            "(object %d)\n", object_count);
    return false;
  }
  if (nsprimitives >= nprimitives) {
    fprintf(stderr, "Bad simplified number of primitives, %lld "
            "(object %d)\n", (long long signed int)nsprimitives + 1,
            object_count);
    return false;
  }
  ++nsprimitives;

  int32_t nsvertices;
  if (!reader_fread_int32(&nsvertices, r)) {
    fprintf(stderr, "Failed to read simplified number of vertices "
            "(object %d)\n", object_count);
    return false;
  }
  if (nsvertices < 0 || nsvertices >= nvertices) {
    fprintf(stderr, "Bad simplified number of vertices, %lld "
            "(object %d)\n", (long long signed int)nsvertices + 1,
            object_count);
    return false;
  }
  ++nsvertices;

  if (reader_fseek(r, PaddingBeforeClipDist, SEEK_CUR)) {
    fprintf(stderr, "Failed to seek clip distance (object %d)\n",
            object_count);
    return false;
  }

  int32_t clip_dist;
  if (!reader_fread_int32(&clip_dist, r)) {
    fprintf(stderr, "Failed to read clip distance (object %d)\n",
            object_count);
    return false;
  }
  if (clip_dist < 0) {
    fprintf(stderr, "Bad clip distance, %" PRId32 " (object %d)\n",
            clip_dist, object_count);
    return false;
  }

  int32_t primitive_style;
  if (!reader_fread_int32(&primitive_style, r)) {
    fprintf(stderr, "Failed to read primitive style (object %d)\n",
            object_count);
    return false;
  }
  if ((primitive_style != Outline_None) &&
      (primitive_style != Outline_Black) &&
      (primitive_style != Outline_Blue)) {
    fprintf(stderr, "Bad primitive style, %" PRId32 " (object %d)\n",
            primitive_style, object_count);
    return false;
  }

  vertex_array_clear(varray);

  if (!parse_vertices(r, object_count, varray,
                      nvertices, nsvertices, flags)) {
    return false;
  }

  for (int g = 0; g < Group_Count; ++g) {
    group_delete_all((*groups) + g);
  }

  /* Objects 37 and 38 have bad primitive counts */
  if ((nprimitives > 0) && (nsprimitives > 0)) {
    if (!parse_primitives(r, object_count, varray, groups, simple_dist,
                          nprimitives, nsprimitives, thick, flags)) {
      return false;
    }
  }

  if (out != NULL) {
    /* In cases of overlapping coplanar polygons,
       split the underlying polygon */
    if (flags & FLAGS_CLIP_POLYGONS) {
      const int group_order[] = {Group_Simple, Group_Complex};
      if (!clip_polygons(varray, *groups, group_order,
                         ARRAY_SIZE(group_order),
                         (flags & FLAGS_VERBOSE) != 0)) {
        fprintf(stderr,
                "Clipping of overlapping coplanar polygons failed\n");
        return false;
      }
    }

    /* Mark the vertices in preparation for culling unused ones. */
    mark_vertices(varray, groups, object_count, flags);

    if (!(flags & FLAGS_DUPLICATE)) {
      /* Unmark duplicate vertices in preparation for culling them. */
      if (vertex_array_find_duplicates(varray,
                                       (flags & FLAGS_VERBOSE) != 0) < 0) {
        fprintf(stderr, "Detection of duplicate vertices failed\n");
        return false;
      }
    }

    int vobject;
    if (!(flags & FLAGS_UNUSED) || !(flags & FLAGS_DUPLICATE)) {
      /* Cull unused and/or duplicate vertices */
      vobject = vertex_array_renumber(varray, (flags & FLAGS_VERBOSE) != 0);
      DEBUGF("Renumbered %d vertices\n", vobject);
    } else {
      vobject = vertex_array_get_num_vertices(varray);
      DEBUGF("No need to renumber %d vertices\n", vobject);
    }

    if (fprintf(out, "\no %s\n"
                     "# Simplification distance: %" PRId32 "\n"
                     "# Clip distance: %" PRId32 "\n"
                     "# Primitive style: %s\n",
               object_name, simple_dist, clip_dist,
               style_to_string(primitive_style)) < 0) {
      fprintf(stderr,
              "Failed writing to output file: %s\n",
              strerror(errno));
      return false;
    }

    VertexStyle vstyle = VertexStyle_Positive;
    if (flags & FLAGS_NEGATIVE_INDICES) {
      vstyle = VertexStyle_Negative;
    }

    MeshStyle mstyle = MeshStyle_NoChange;
    if (flags & FLAGS_TRIANGLE_FANS) {
      mstyle = MeshStyle_TriangleFan;
    } else if (flags & FLAGS_TRIANGLE_STRIPS) {
      mstyle = MeshStyle_TriangleStrip;
    }

    if (!output_vertices(out, vobject, varray, -1) ||
        !output_primitives(out, object_name, *vtotal, vobject,
                           varray, *groups, ARRAY_SIZE(*groups),
                           (flags & FLAGS_FALSE_COLOUR) ?
                             get_false_colour : (OutputPrimitivesGetColourFn *)NULL,
                           (flags & FLAGS_HUMAN_READABLE) ?
                             get_human_material : get_material,
                           NULL, vstyle, mstyle)) {
      fprintf(stderr, "Failed writing to output file: %s\n",
              strerror(errno));
      return false;
    }

    *vtotal += vobject;
  }

  if (flags & FLAGS_LIST) {
    if (!*list_title) {
      puts("\nIndex  Name          Verts  Prims  SimpV  SimpP      "
           "Offset        Size");
      *list_title = true;
    }

    const long int obj_size = reader_ftell(r) - obj_start;
    printf("%5d  %-12.12s  %5d  %5d  %5d  %5d  %10ld  %10ld\n",
           object_count, object_name, nvertices, nprimitives, nsvertices,
           nsprimitives, data_start + obj_start, obj_size);
  }

  return true;
}

bool choc_to_obj(Reader * const index, Reader * const models,
                 FILE * const out, const int first, const int last,
                 _Optional const char * const name, const long int data_start,
                 const char * const mtl_file, double const thick,
                 const unsigned int flags)
{
  bool success = true;
  Group groups[Group_Count];
  for (int g = 0; g < Group_Count; ++g) {
    group_init(groups + g);
  }
  VertexArray varray;
  vertex_array_init(&varray);
  int vtotal = 0;

  assert(index != NULL);
  assert(!reader_ferror(index));
  assert(models != NULL);
  assert(!reader_ferror(models));
  assert(first >= 0);
  assert(last == -1 || last >= first);
  assert(mtl_file != NULL);
  assert(thick >= 0);
  assert(!(flags & ~FLAGS_ALL));

  if ((out != NULL) &&
      fprintf(out, "# Chocks Away graphics\n"
                   "# Converted by ChoctoObj "VERSION_STRING"\n"
                   "mtllib %s\n", mtl_file) < 0) {
    fprintf(stderr, "Failed writing to output file: %s\n",
            strerror(errno));
    success = false;
  } else {
    /* Read each object address in turn until reaching the
       end of the file (or error). */
    int32_t address, last_address = 0, first_address = -1;
    bool list_title = false, stop = false;
    int object_count;
    for (object_count = 0; !stop && success; ++object_count) {
      if (!reader_fread_int32(&address, index)) {
        if (reader_ferror(index)) {
          fprintf(stderr, "Failed to read from index file (object %d)\n",
                  object_count);
          success = false;
        }
        break;
      }
      if (address < last_address) {
        fprintf(stderr, "Bad address %" PRId32 " (0x%" PRIx32 ") "
                "for object %d in index\n", address, address, object_count);
        success = false;
        break;
      }

      last_address = address;

      if (first_address < 0) {
        first_address = address;
      }

      assert(address >= first_address);
      assert(first_address >= 0);
      long int const offset = (long int)address - first_address;

      if (flags & FLAGS_VERBOSE) {
        printf("Object %d has address 0x%" PRIx32 ", offset %ld (0x%lx)\n",
               object_count, address, offset, offset);
      }

      /* When only summarizing, we need to enumerate the entire index but
         not try to dereference any of the addresses therein. */
      if ((flags & FLAGS_SUMMARY) && !(flags & FLAGS_LIST)) {
        continue;
      }

      /* Is this object in the selected range? */
      if ((object_count < first) && (first != -1)) {
        continue;
      }

      /* Is this object the named one? */
      const char * const object_name = (flags & FLAGS_EXTRA_MISSIONS) ?
                                       get_obj_name_extra(object_count) :
                                       get_obj_name(object_count);

      if (name != NULL) {
        if (!strcmp(&*name, object_name)) {
          /* Stop after finding the named object (assuming there are
             no others of the same name) */
          stop = true;
        } else {
          continue;
        }
      }

      if ((last != -1) && (object_count >= last)) {
        /* Stop after the end of the specified range of object numbers */
        stop = true;
      }

      if (offset < data_start) {
        if (flags & FLAGS_VERBOSE) {
          printf("Object %d at offset %ld (0x%lx) "
                 "precedes input at offset %ld (0x%lx)\n",
                 object_count, offset, offset, data_start, data_start);
        }
        continue;
      }

      long int const file_pos = offset - data_start;
      int err = reader_fseek(models, file_pos, SEEK_SET);
      if (!err) {
        /* fseek doesn't return an error when seeking beyond the end
           of a file. */
        const int c = reader_fgetc(models);
        if (c == EOF) {
          err = 1;
        } else {
          if (reader_ungetc(c, models) == EOF) {
            fprintf(stderr, "Failed to push back first byte of object %d\n",
                    object_count);
            success = false;
            break;
          }
        }
      }

      if (err) {
        fprintf(stderr, "Failed to seek object %d at offset %ld (0x%lx), "
                        "file position %ld (0x%lx)\n",
                object_count, offset, offset, file_pos, file_pos);
        success = false;
        break;
      }

      if (flags & FLAGS_VERBOSE) {
        printf("Found object %d at file position %ld (0x%lx)\n",
               object_count, file_pos, file_pos);
      }

      success = process_object(models, out, object_name, object_count,
                               &varray, &groups, &vtotal, &list_title,
                               thick, data_start, flags);
    }

    if (success && (flags & FLAGS_SUMMARY)) {
      printf("\nFound %d object address%s\n",
             object_count, object_count != 1 ? "es" : "");
    }
  }

  for (int g = 0; g < Group_Count; ++g) {
    group_free(groups + g);
  }
  vertex_array_free(&varray);

  return success;
}
