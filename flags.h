/*
 *  ChoctoObj - Converts Chocks Away graphics to Wavefront format
 *  Flags controlling generation of Wavefront object files
 *  Copyright (C) 2018 Christopher Bazley
 */

#ifndef FLAGS_H
#define FLAGS_H

#define FLAGS_VERBOSE            (1u<<0)  /* emit information about processing */
#define FLAGS_LIST               (1u<<1)  /* list objects on standard output */
#define FLAGS_SUMMARY            (1u<<2)  /* summarize objects on standard output */
#define FLAGS_NEGATIVE_INDICES   (1u<<3)  /* emit negative vertex indices */
#define FLAGS_HIDDEN_POLYGONS    (1u<<4)  /* emit hidden polygons */
#define FLAGS_UNUSED             (1u<<5)  /* emit unreferenced vertices */
#define FLAGS_TRIANGLE_FANS      (1u<<6)  /* split complex polygons into fans */
#define FLAGS_TRIANGLE_STRIPS    (1u<<7)  /* split complex polygons into strips */
#define FLAGS_CLIP_POLYGONS      (1u<<8)  /* clip underlying polygons */
#define FLAGS_FALSE_COLOUR       (1u<<9)  /* assign false primitive colours */
#define FLAGS_DUPLICATE          (1u<<10) /* emit duplicate vertices */
#define FLAGS_HUMAN_READABLE     (1u<<11) /* use human-readable material names */
#define FLAGS_SIMPLE             (1u<<13) /* emit simple objects */
#define FLAGS_EXTRA_MISSIONS     (1u<<14) /* emit extra missions object names */
#define FLAGS_FLIP_BACKFACING    (1u<<15) /* flip backfacing polygons */
#define FLAGS_ALL                ((1u<<16)-1)

#endif /* FLAGS_H */
