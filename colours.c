/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Colour names
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

/* Local header files */
#include "colours.h"
#include "misc.h"

const char *get_colour_name(const int colour)
{
  static const char * const colour_names[] =
  {
     "black",
     "darkmaroon",
     "darknavy",
     "darkpurple",
     "maroon",
     "mediumred",
     "tyrianpurple",
     "crimson",
     "darkgreen",
     "darkolive",
     "darkteal",
     "darkgrey",
     "brown",
     "mahogany",
     "cordovan",
     "brickred",
     "green",
     "avocado",
     "pigmentgreen",
     "ferngreen",
     "olive",
     "harvestgold",
     "darktan",
     "peru",
     "mediumgreen",
     "napiergreen",
     "darkpastelgreen",
     "limegreen",
     "applegreen",
     "peridot",
     "yellowgreen",
     "oldgold",
     "navy",
     "indigo",
     "mediumblue",
     "violetblue",
     "purple",
     "mediumvioletred",
     "darkviolet",
     "deepmagenta",
     "mediumelectricblue",
     "darkslateblue",
     "royalazure",
     "pigmentblue",
     "plum",
     "mulberry",
     "lavenderindigo",
     "deepfuchsia",
     "teal",
     "dustyteal",
     "honolulublue",
     "celestialblue",
     "grey",
     "oldrose",
     "ube",
     "pastelviolet",
     "caribbeangreen",
     "mint",
     "darkturquoise",
     "mediumturquoise",
     "darkseagreen",
     "lightbeige",
     "pearlaqua",
     "lightgrey"
  };
  assert(colour >= 0);
  assert((size_t)colour < ARRAY_SIZE(colour_names));
  return colour_names[colour];
}
