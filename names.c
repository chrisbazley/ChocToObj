/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Chocks Away object names
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
#include <stdio.h>

/* Local header files */
#include "names.h"
#include "misc.h"

const char *get_obj_name(const int index)
{
  static char buffer[64];
  static struct {
    int num;
    const char *string;
  } names[] = {
    /* Although many other object meshes are recognizable, these are
       the only named targets in the original 'Chocks Away'. */
    { 0, "gun"},           /* 'GROUND GUN BASE' */
    { 1, "store"},         /* 'STORE BUILDING' */
    { 2, "tank"},          /* 'TANK' */
    { 3, "headquarters"},  /* 'HEAD QUARTERS' */
    { 4, "tower"},         /* 'CONTROL TOWER' */
    { 5, "boat"},          /* 'PATROL BOAT' */
    { 18, "tiger"},        /* 'TIGER MOTH' */
    { 19, "twin" },        /* 'FOKKER V7 TWIN' */
    { 22, "gotha" },       /* 'GOTHA G IV BOMBER' */
    { 23, "s_tiger" },
    { 24, "s_twin" },
    { 25, "s_gotha" },     /* ...shadows... */
    { 26, "s_eindecker" },
    { 27, "s_scout" },
    { 28, "s_triplane" },
    { 29, "eindecker" },   /* 'FOKKER EINDECKER IV' */
    { 30, "triplane" },    /* 'FOKKER VIII TRIPLANE' */
    { 31, "scout" },       /* 'ALBATROS DIII SCOUT' */
  };
  _Optional const char *n = NULL;
  assert(index >= 0);

  for (size_t i = 0; (n == NULL) && (i < ARRAY_SIZE(names)); ++i) {
    if (names[i].num == index) {
      n = names[i].string;
    }
  }

  if (n == NULL) {
    sprintf(buffer, "chocks_%d", index);
    n = buffer;
  }

  return &*n;
}

const char *get_obj_name_extra(const int index)
{
  static struct {
    int num;
    const char *string;
  } names[] = {
    /* Although many other object meshes are recognizable, these are
       the only additional named targets in the 'Extra Missions'. */
    { 46, "bridge" },       /* 'BRIDGE' */
    { 52, "carrier" },      /* 'AIRCRAFT CARRIER' */
    { 54, "yacht" },        /* 'YACHT' */
    { 68, "factory" },      /* 'FACTORY' */
    { 72, "airship" },      /* 'AIRSHIP' */
    { 73, "balloon" },      /* 'BARRAGE BALLOON' */
    { 78, "terminal" },     /* 'CONTROL TERMINAL' */
    { 79, "tanker" },       /* 'OIL TANKER' */
    { 81, "gunboat"},       /* 'GUN BOAT' */
    { 85, "train"},         /* 'TRAIN' */
    { 77, "biplane" },      /* 'FOKKER DE5 BIPLANE' */
    { 75, "triengine" },    /* 'FOKKER V3 TRIENGINE' */
    { 74, "cargo" },        /* 'CARGO AIRCRAFT' */
    { 87, "station" },      /* 'RAILWAY STATION' */
    { 102, "s_biplane" },
    { 103, "s_triengine" }, /* ...shadows... */
    { 104, "s_cargo" },
    { 107, "ground_jet" },  /* 'JET FIGHTER' */
    { 108, "jet" }          /* 'JET FIGHTER' */
  };
  _Optional const char *n = NULL;
  assert(index >= 0);

  for (size_t i = 0; (n == NULL) && (i < ARRAY_SIZE(names)); ++i) {
    if (names[i].num == index) {
      n = names[i].string;
    }
  }

  if (n == NULL) {
    n = get_obj_name(index);
  }

  return &*n;
}
