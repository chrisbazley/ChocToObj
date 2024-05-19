# ChocToObj
Chocks Away to Wavefront convertor

(C) Christopher Bazley, 2018

Version 0.03 (21 Apr 2020)

-----------------------------------------------------------------------------
 1   Introduction and Purpose
-----------------------------

  'Chocks Away' is a flight simulator for Acorn Archimedes computers, written
by Andrew Hutchings. It was published in 1990 by The Fourth Dimension (and
followed by 'Extra Missions' in 1991). It is fondly remembered by many
because of its smoothly animated 3D graphics, simple controls and inclusion
of a split-screen dogfight mode. The game world is drawn using a combination
of flat-shaded polygon meshes, lines and points. Its 3D models of aircraft,
buildings, bridges and other structures are primitive but I think they have
a certain charm.

  This command-line program can be used to convert object models belonging to
'Chocks Away' from their original (optionally compressed) format into the
simple object geometry format developed by Wavefront for their Advanced
Visualizer software. Wavefront OBJ format is a de-facto standard for 3D
computer graphics and it has the advantage of being human-readable.

  The colour and other visual properties of objects are defined separately
from the object geometry (OBJ) file in a companion file known as a material
library (MTL) file. The supplied MTL file defines all of the colours in the
default RISC OS 256-colour palette as named materials, assuming a constant
colour illumination model (no texture map, ambient or specular reflectance).

-----------------------------------------------------------------------------
2   Requirements
----------------

  The supplied executable files will only work on RISC OS machines. They have
not been tested on any version of RISC OS earlier than 4, although they
should also work on earlier versions provided that a suitable version of the
'SharedCLibrary' module is active. They should be compatible with 32 bit
versions of RISC OS.

  You will obviously require a copy of the game 'Chocks Away' or 'Chocks
Away: Extra Missions' from which to rip the graphics.

  Index and model data files for 'Chocks Away' can be found inside the
!Chocks application. The actual models are stored in a file named
!Chocks.Maps.Land. This data is indexed by an array of 200 addresses stored
in a second file named !Chocks.Maps.Obj3D.

```
!Chocks
   `--Maps
       |--Land
       `--Obj3D
```

  Index and model data files for 'Extra Missions' can be found inside the
!Maps_2 application. This game requires additional models for some maps:
instead of a single Obj3D file, alternative index files numbered from
!Maps_2.Things.Obj3D0 to Obj3DF store between 109 and 159 model addresses.
Model data in common between all maps is still stored in a single file named
!Maps_2.Land and any extra model data is stored in files numbered LandEx0
to LandExF (many of which have length zero).

```
!Maps_2
   |--Land
   |--LandEx0
   |--LandEx1
   |--LandEx..
   `--Things
        |--Obj3D0
        |--Obj3D1
        `--Obj3D..
```
-----------------------------------------------------------------------------
3   Quick Guide
---------------

  Ensure that the !Chocks application has been 'seen' by the Filer, so that
the system variable Chocks$Dir has been set.

  Hold down the Shift key and double-click on the !Chocks application to
open it.

  Set the current selected directory to that containing the 'ChocToObj'
program. On RISC OS 6, that can be done by opening the relevant directory
display and choosing 'Set work directory' from the menu (or giving the
directory display the input focus and then pressing F11).

  Press Ctrl-F12 to open a task window and then invoke the conversion program
using the following command:
```
  *ChocToObj -list <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```
  This should list all of the objects addressed by the compressed index
file 'Obj3D'. One of those should be named 'tiger', which is a 3D model
of the Tiger Moth biplane flown by the player.

  To convert the Tiger Moth model into a Wavefront OBJ file named
'tiger/obj', use the following command:
```
  *ChocToObj -name tiger -human <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D tiger/obj
```
  The '-name' argument selects which object is converted and the '-human'
argument enables generation of human-readable material names such as
'maroon_0' in the output file.

  To convert the same model from 'Chocks Away: Extra Missions', ensure that
the !Maps_2 application has been 'seen' by the Filer, then use the following
command:
```
  *ChocToObj -extra -raw -name tiger -human <ExtraMaps$Dir>.Land <ExtraMaps$Dir>.Things.Obj3D0 tiger/obj
```
  The '-raw' argument disables decompression of the model data and index
files whilst the '-extra' argument (strictly redundant in this case) allows
naming of models only named in Extra Missions.

-----------------------------------------------------------------------------
4   Usage information
---------------------

4.1 Command line syntax
-----------------------

  The basic syntax of the program is summarised below. All switches are
optional. Switch names may be abbreviated provided that doing so does not
make the command ambiguous.
```
  *ChocToObj  [switches] <model-file> [<index-file> [<output-file>]]
```

4.2 Input and output
--------------------

Switches:
```
  -raw                Model and index files are uncompressed raw data
  -outfile <file>     Write output to the named file instead of stdout
```
  When invoking ChocToObj, you must always specify the name of a model data
file. Without this, it would only be possible to enumerate the number of
models and their addresses in memory (from the index).

  The other input to ChocToObj is an array of four-byte little-endian model
data addresses (the index). Consequently, it's possible for several object
numbers to alias the same model data.

  Strictly, one would need to know the address at which the model data file
will be loaded in order to correctly interpret the index; in practice, the
lowest address in the index usually coincides with the start of the model
data.

  An output file name can be specified after the input file name or, by means
of the '-outfile' parameter, before the model data file name.

  If no index file is specified then addresses are read from 'stdin' (the
standard input stream; keyboard unless redirected). If no output file is
specified then OBJ-format output is written to 'stdout' (the standard output
stream; screen unless redirected).

  All of the following examples read the index from a file named 'obj3d' and
write output to a file named 'chocks/obj':
```
  *ChocToObj land obj3d chocks/obj
  *ChocToObj -outfile chocks/obj land obj3d
  *ChocToObj -outfile chocks/obj land <obj3d
  *ChocToObj land obj3d >chocks/obj
```
  Under UNIX-like operating systems, output can be piped directly into
another program.

  Search for a named object in a compressed index file named 'obj3d':
```
  ChocToObj land obj3d | grep -i 'gotha'
```
  By default, all input is assumed to be compressed. The switch '-raw'
allows uncompressed input, which is useful for converting object models
belonging to 'Chocks Away: Extra Missions'.

  It isn't possible to mix compressed and uncompressed input, for example by
using a compressed index with an uncompressed model data file.

4.3 Model data file
-------------------

Switches:
```
  -last N    Last object number to convert or list
  -offset N  Signed byte offset to start of model data in file
```
  Not all of the models for 'Chocks Away: Extra Missions' are stored in a
single file; models in common between all maps are stored in a primary file
whilst extra models required for specific maps are stored in a secondary
file (which is appended to the first upon loading). The index can reference
models in either file.

  This presents a problem because ChocToObj can only read from one model
file at at time. A solution is provided in the form of the '-last N' and
'-offset N' parameters.

  The '-last N' parameter is used to prevent ChocToObj reading beyond the
end of the primary model data file. The object number specified should be
one less than that reported in the error message that results from omitting
this parameter (e.g. "Failed to seek object 109 at offset ...").

  The '-offset N' parameter is used to specify that the secondary model data
file is not loaded at the lowest address in the index. The offset specified
should be the (decompressed) size of the primary model data file.

  The following example illustrates how these concepts can be combined to
list all of the models for map 4 of 'Chocks Away: Extra Missions', albeit in
piecemeal fashion:
```
  *FileInfo <ExtraMaps$Dir>.Land
  Land         WR/WR   Data      21:50:19.02 14-Apr-2018 0000C478
  *ChocToObj -list -extra -raw -last 108 <ExtraMaps$Dir>.Land <ExtraMaps$Dir>.Things.Obj3D4
  *ChocToObj -list -extra -raw -offset 0xC478 <ExtraMaps$Dir>.LandEx4 <ExtraMaps$Dir>.Things.Obj3D4
```

4.4 Object selection
--------------------

Switches:
```
  -index N     Object number to convert or list (default is all)
  -first N     First object number to convert or list
  -last N      Last object number to convert or list
  -name <name> Object name to convert or list (default is all)
  -extra       Enable object names from Extra Missions
```
  The model data index can be filtered using the '-index' or '-first' and
'-last' parameters to select a single object or range of objects to be
processed. Using the '-index' parameter is equivalent to setting the first
and last numbers to the same value. The lowest object number is 0 and the
length of the index dictates the valid range of object numbers.

  The index can also be filtered using the '-name' parameter to select a
single object to be processed. The '-extra' switch enables additional object
names for things that are only named in 'Chocks Away: Extra Missions'. Any
filter specified applies when listing object definitions as well as when
converting them.

  If no range of object numbers and no name is specified then all entries in
the index are used.

  Convert the first object in file 'Obj3D', outputting to the screen:
```
  *ChocToObj -index 0 <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```

  If an object name is specified then the single object of the given name
is processed (provided that it falls within the specified range of object
numbers, if any).

  Convert the Gotha G IV bomber object in file 'Obj3D' (see below for object
naming):
```
  *ChocToObj -name gotha <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```

Named objects in 'Chocks Away':
```
  Number Name          Object
  0      gun           Ground gun base
  1      store         Store building
  2      tank          Tank
  3      headquarters  Headquarters ('Head quarters' in the game)
  4      tower         Control tower
  5      boat          Patrol boat
  18     tiger         Tiger moth
  19     twin          Fokker V7 twin
  22     gotha         Gotha G IV bomber
  23     s_tiger       Shadow of tiger moth
  24     s_twin        Shadow of Fokker V7 twin
  25     s_gotha       Shadow of Gotha G IV bomber
  26     s_eindecker   Shadow of Fokker Eindecker IV
  27     s_scout       Shadow of Albatros DIII scout
  28     s_triplane    Shadow of Fokker VIII triplane
  29     eindecker     Fokker Eindecker IV
  30     triplane      Fokker VIII triplane
  31     scout         Albatros DIII scout
```

Additional named objects in 'Chocks Away: Extra Missions':
```
  Number Name        Object
  46     bridge      Bridge
  52     carrier     Aircraft carrier
  54     yacht       Yacht
  68     factory     Factory
  72     airship     Airship
  73     balloon     Barrage balloon
  78     terminal    Control terminal
  79     tanker      Oil tanker
  81     gunboat     Gunboat ('Gun boat' in the game)
  85     train       Train
  77     biplane     Fokker DE5 biplane
  75     triengine   Fokker V3 triengine
  74     cargo       Cargo aircraft
  87     station     Railway station
  102    s_biplane   Shadow of Fokker DE5 biplane
  103    s_triengine Shadow of Fokker V3 triengine
  104    s_cargo     Shadow of cargo aircraft
  107    ground_jet  Jet fighter
  108    jet         Jet fighter
```

4.5 Listing and summarizing objects
-----------------------------------

Switches:
```
  -list     List objects instead of converting them
  -summary  Summarize objects instead of converting them
```
  If the switch '-list' is used then ChocToObj lists object definitions
instead of converting them to Wavefront OBJ format. Only object definitions
matching any filter specified using the '-index', '-first', '-last', and
'-name' parameters are listed.

  No output file can be specified because no OBJ-format output is generated
and the object list or summary is always sent to the standard output stream.

  List all object definitions indexed by file 'Obj3D':
```
  *ChocToObj -list <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```

  Output is a table with the following format:
```
Index  Name          Verts  Prims  SimpV  SimpP      Offset        Size
    0  gun               8      5      6      1           0         208
```

  If the switch '-summary' is used then ChocToObj summarizes object
definitions instead of converting them to Wavefront OBJ format. This switch
can be used in conjunction with '-list', in which case the summary is
displayed after the list.

  Summarize all object definitions indexed by file 'Obj3D':
```
  *ChocToObj -summary <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```

  Output is a total in the following format:
```
Found 200 objects in the input
```

4.6 Materials
-------------

Switches:
```
  -mtllib name   Specify a material library file (default sf3k.mtl)
  -human         Output readable material names
  -false         Assign false colours for visualization
```

  By default, ChocToObj emits 'usemtl' commands that refer to colours in
the standard RISC OS 256-colour palette by derived material names such as
'riscos_31'. (This naming convention is for compatibility with a similar
program, SF3KtoObj.)

  Convert the player's aircraft, outputting colour numbers to the screen:
```
  *ChocToObj -index 18 <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```

```
# 2 primitives
g tiger tiger_0
usemtl riscos_19
f 27 26 25 24
usemtl riscos_24
f 1 2 4 3
...
```

  If the switch '-human' is used then ChocToObj instead generates human-
readable material names such as 'black_0'.

  Convert the player's aircraft, outputting readable material names to the
screen:
```
  *ChocToObj -index 18 -human <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```
  
```
# 2 primitives
g tiger tiger_0
usemtl maroon_3
f 27 26 25 24
usemtl tyrianpurple_0
f 1 2 4 3
...
```

  By default, ChocToObj emits a 'mtllib' command which references 'sf3k.mtl'
as the material library file to be used when drawing objects; this is the
same as the name of the supplied MTL file.

  An alternative material library file can be specified using the switch
'-mtllib'. The named file is not created, read or written by ChocToObj.

  False colours can be assigned to help visualise boundaries between
polygons, especially between coplanar polygons of the same colour. This
mode, which is mainly useful for debugging, is enabled by specifying the
switch '-false'.

  When false colours are enabled, the colours in the input file are ignored.

4.7 Clipping
------------

Switches:
```
  -clip      Clip overlapping coplanar polygons
  -thick N   Line thickness (N=0..100, default 0)
```
  Some objects are liable to suffer from a phenomenon known as "Z-fighting"
if they are part of a scene rendered using a depth (Z) buffer. It is caused
by overlapping faces in the same geometric plane and typically manifests as
a shimmering pattern in a rendered image. Essentially two or more polygons
occupy the same points in 3D space.

  The game uses painter's algorithm to ensure that overlapping objects are
drawn correctly (drawing more distant objects before nearer ones), instead
of using a depth buffer. It also draws the polygons of each object in a
predictable order, which allows overlapping polygons to be used as decals
(e.g. as doors and windows on buildings).

  If the switch '-clip' is used then the rearmost of two overlapping polygons
is split by dividing it along the line obtained by extending one edge of the
front polygon to infinity in both directions. This process is repeated until
no two edges intersect. Any polygons that are fully hidden (typically behind
decals) are deleted.

  The following diagrams illustrate how one polygon (B: 1 2 3 4) overlapped
by another (A: 5 6 7 8) is split into five polygons (B..F) during the
clipping process. The last polygon (F) is then deleted because it duplicates
the overlapping polygon (A).

```
     Original         First split       Second split
  (A overlaps B)     (A overlaps C)    (A overlaps D)
                               :
  3_____________2   3__________9__2   3__________9__2
  |      B      |   |          |  |   |      C   |  |
  |  7_______6  |   |          |  | ..|__________6..|..
  |  |   A   |  |   |      C   |B | 11|          |B |
  |  |_______|  |   |          |  |   |      D   |  |
  |  8       5  |   |          |  |   |          |  |
  |_____________|   |__________|__|   |__________|__|
  4             1   4          10 1   4          10 1
                               :
    Third split       Fourth split     Final clipped
  (A overlaps E)     (A overlaps F)     (F deleted)
  3__:_______9__2   3__________9__2   3__________9__2
  |  :   C   |  |   |      C   |  |   |      C   |  |
11|__7_______6  | 11|__7_______6  | 11|__7_______6  |
  |  |       |B |   |D |   F   |B |   |D |   A   |B |
  |D |   E   |  | ..|..|_______|..|.. |  |_______|  |
  |  |       |  |   |  8   E   5  |   |  8   E   5  |
  |__|_______|__|   |__|_______|__|   |__|_______|__|
  4  12      10 1   4  12      10 1   4  12      10 1
     :
```

  Some of the game's models include lines which are coplanar with (and
completely contained by) a polygon that is drawn first. Examples include
the middle stripe on a road. This causes "Z-fighting" between the line and
the polygon.

  If the parameter '-thick N' is used then such lines will be replaced with
rectangular polygons with the same length and centreline as the original
line but a width specified by the user. This serves two purposes:

1. It allows the coplanar polygon clipper (if enabled) to split the rearmost
   polygon, thereby solving the "Z-fighting" issue.

2. It prevents the line from appearing disproportionately thin if the
   resolution of the frame buffer is higher than that used by the game.

4.8 Normal correction
---------------------

Switches:
```
  -flip      Flip back-facing polygons coplanar with z=0
```
  The order in which vertices are specified in a primitive definition
determines the direction of a polygon's normal vector and therefore whether
or not it is facing the camera in any given scene.

  The game draws primitives in the order they are defined without using a
depth buffer. Some models are sufficiently complex that back-face culling is
required to prevent near polygons being overdrawn by far polygons; others
require polygons to be visible from both sides.

  Back-face culling is enabled per object instance in the game's map data,
therefore it isn't possible to predict whether the order of vertices in a
polygon definition is significant without knowing the model's intended
usage. One aerodrome has two runways and one taxiway defined clockwise
(facing the sky) but two other taxiways defined anti-clockwise (facing the
earth).

  ChocToObj can infer that polygons should not be facing the earth if they
belong to a model in which all primitives are coplanar with the xy plane at
z=0. The switch '-flip' reverses the order of vertices for such polygons to
make them face the sky instead.

4.9 Model simplification
------------------------

Switches:
```
  -simple  Output simplified models
```
  The game supports two levels of detail for each model, selected according
to its distance from the camera (e.g. a distant house might be drawn without
its windows). By default, ChocToObj outputs primitives that belong to both
high-and low-detail versions of a model in group 0 and any remaining
primitives (required only for the high-detail model) in group 1.

  If the switch '-simple' is used then ChocToObj reads only vertices and
primitives belonging to the simplified version of each model. The effect
is not quite as straightforward as only outputting primitives in group 0: if
a polygon is always simplified at the same distance as (or closer than) the
distance at which the whole model would be simplified then that polygon must
be replaced with a line. In many cases this is necessary to avoid references
to vertices that are not included in a simplified model's vertex count.

4.10 Output of faces
--------------------

Switches:
```
  -fans      Split complex polygons into triangle fans
  -strips    Split complex polygons into triangle strips
  -negative  Use negative vertex indices
```
  The Wavefront OBJ format specification does not restrict the maximum
number of vertices in a face element. Nevertheless, some programs cannot
correctly display faces with more than three vertices. Two switches, '-fans'
and '-strips', are provided to split complex polygons into triangles.

```
       Original          Triangle fans       Triangle strips
     3_________2          3_________2          3_________2
     /         \          / `-._    \          /\`-._    \
    /           \        /      `-._ \        /  `\  `-._ \
 4 /             \ 1  4 /___________`-\ 1  4 /     \.    `-\ 1
   \             /      \         _.-`/      \`-._   \     /
    \           /        \    _.-`   /        \   `-. \.  /
     \_________/          \.-`______/          \_____`-.\/
     5         6          5         6          5         6

 f 1 2 3 4 5 6        f 1 2 3              f 1 2 3
                      f 1 3 4              f 6 1 3
                      f 1 4 5              f 6 3 4
                      f 1 5 6              f 5 6 4
```

  Vertices in a face element are normally indexed by their position in the
output file, counting upwards from 1. If the output comprises more than one
object definition then it can be more useful to count backwards from the
most recent vertex definition, which is assigned index -1. The '-negative'
switch enables this output mode, which allows object definitions to be
separated, extracted or rearranged later.

  Convert the Fokker VIII triplane with positive vertex indices:
```
  *ChocToObj -index 30 <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```
  
```
# 1 primitives
g triplane triplane_0
usemtl riscos_67
f 27 26 25 24

# 22 primitives
g triplane triplane_1
usemtl riscos_73
f 34 35 37 36
f 1 2 4 3
...
```

  Convert the same object with negative vertex indices:
```
  *ChocToObj -index 30 -neg <Chocks$Dir>.Maps.Land <Chocks$Dir>.Maps.Obj3D
```
  
```
# 1 primitives
g triplane triplane_0
usemtl riscos_67
f -11 -12 -13 -14

# 22 primitives
g triplane triplane_1
usemtl riscos_73
f -4 -3 -1 -2
f -37 -36 -34 -35
...
```

4.11 Hidden data
----------------

Switches:
```
  -unused     Include unused vertices in the output
  -duplicate  Include duplicate vertices in the output
```
  It's common for model data to include vertex definitions that are not
referenced by any primitive definition. Such vertices are not included in
the output unless the '-unused' switch is used.

  Some object definitions also include two or more vertices with the same
coordinates, where both vertices are referenced by primitives. An example is
vertices 6 and 7 of the aircraft carrier model. Such pairs of vertices are
automatically merged unless the '-duplicate' switch is specified.

4.12 Getting diagnostic information
-----------------------------------

Switches:
```
  -time               Show the total time for each file processed
  -verbose or -debug  Emit debug information (and keep bad output)
```
  If either of the switches '-verbose' and '-debug' is used then the program
emits information about its internal operation on the standard output
stream. However, this makes it slower and prevents output being piped to
another program.

  If the switch '-time' is used then the total time for each file processed
(to centisecond precision) is printed. This can be used independently of
'-verbose' and '-debug'.

  When debugging output or the timer is enabled, you must specify an output
file name. This is to prevent OBJ-format output being sent to the standard
output stream and becoming mixed up with the diagnostic information.

-----------------------------------------------------------------------------
5   Colour names
----------------

  The colour names were taken from a variety of online sources including
Wikipedia articles, W3C standards, the X Window System, and the results of
the web comic XKCD's colour naming survey.

| Index | Web RGB | Name                 | Notes                         |
|-------|---------|----------------------|-------------------------------|
|     0 | #000000 | black              * | Identical to HTML 'black'     |
|     4 | #440000 | darkmaroon           | By analogy with 'darkred'     |
|     8 | #000044 | darknavy             | By analogy with 'darkblue'    |
|    12 | #440044 | darkpurple           | By analogy with 'darkmagenta' |
|    16 | #880000 | maroon             * | Like HTML colour #800000      |
|    20 | #cc0000 | mediumred            | By analogy with 'mediumblue'  |
|    24 | #880044 | tyrianpurple         | Like Wikipedia colour #66023c |
|    28 | #cc0044 | crimson            * | Like X11 colour #dc143c       |
|    32 | #004400 | darkgreen          + | Like X11 colour #006400       |
|    36 | #444400 | darkolive            | Like XKCD colour #373e02      |
|    40 | #004444 | darkteal             | Like XKCD colour #014d4e      |
|    44 | #444444 | darkgrey           + | Darker than X11 'dimgray'     |
|    48 | #884400 | brown              * | Like Wikipedia colour #964b00 |
|    52 | #cc4400 | mahogany             | Like Wikipedia colour #c04000 |
|    56 | #884444 | cordovan             | Like Wikipedia colour #893f45 |
|    60 | #cc4444 | brickred             | Like Wikipedia colour #cb4154 |
|    64 | #008800 | green              * | Like HTML colour #008000      |
|    68 | #448800 | avocado              | Like Wikipedia colour #568203 |
|    72 | #008844 | pigmentgreen         | Like Wikipedia colour #00a550 |
|    76 | #448844 | ferngreen            | Like Wikipedia colour #4f7942 |
|    80 | #888800 | olive              * | Like HTML colour #808000      |
|    84 | #cc8800 | harvestgold          | Like Wikipedia colour #da9100 |
|    88 | #888844 | darktan              | Like Wikipedia colour #918151 |
|    92 | #cc8844 | peru               * | Like X11 colour #cd853f       |
|    96 | #00cc00 | mediumgreen          | By analogy with 'mediumblue'  |
|   100 | #44cc00 | napiergreen          | Like Wikipedia colour #2a8000 |
|   104 | #00cc44 | darkpastelgreen      | Like Wikipedia colour #03c03c |
|   108 | #44cc44 | limegreen          * | Like X11 colour #32cd32       |
|   112 | #88cc00 | applegreen           | Like Wikipedia colour #8db600 |
|   116 | #cccc00 | peridot              | Like Wikipedia colour #e6e200 |
|   120 | #88cc44 | yellowgreen        * | Like X11 colour #9acd32       |
|   124 | #cccc44 | oldgold              | Like Wikipedia colour #cfb53b |
|   128 | #000088 | navy               * | Like HTML colour #000080      |
|   132 | #440088 | indigo             * | Like X11 colour #4b0082       |
|   136 | #0000cc | mediumblue         * | Like X11 colour #0000cd       |
|   140 | #4400cc | violetblue           | Like XKCD colour #510ac9      |
|   144 | #880088 | purple             * | Like HTML colour #800080      |
|   148 | #cc0088 | mediumvioletred    * | Like X11 colour #c71585       |
|   152 | #8800cc | darkviolet         * | Like X11 colour #9400d3       |
|   156 | #cc00cc | deepmagenta          | Like Wikipedia colour #cc00cc |
|   160 | #004488 | mediumelectricblue   | Like Wikipedia colour #035096 |
|   164 | #444488 | darkslateblue      * | Like X11 colour #483d8b       |
|   168 | #0044cc | royalazure           | Like Wikipedia colour #0038a8 |
|   172 | #4444cc | pigmentblue          | Like Wikipedia colour #333399 |
|   176 | #884488 | plum               + | Like Wikipedia colour #8e4585 |
|   180 | #cc4488 | mulberry             | Like Wikipedia colour #c54b8c |
|   184 | #8844cc | lavenderindigo       | Like Wikipedia colour #9457eb |
|   188 | #cc44cc | deepfuchsia          | Like Wikipedia colour #c154c1 |
|   192 | #008888 | teal               * | Like HTML colour #008080      |
|   196 | #448888 | dustyteal            | Like XKCD colour #4c9085      |
|   200 | #0088cc | honolulublue         | Like Wikipedia colour #007fbf |
|   204 | #4488cc | celestialblue        | Like Wikipedia colour #4997d0 |
|   208 | #888888 | grey               * | Like HTML colour #808080      |
|   212 | #cc8888 | oldrose              | Like Wikipedia colour #c08081 |
|   216 | #8888cc | ube                  | Like Wikipedia colour #8878c3 |
|   220 | #cc88cc | pastelviolet         | Like Wikipedia colour #cb99c9 |
|   224 | #00cc88 | caribbeangreen       | Like Wikipedia colour #00cc99 |
|   228 | #44cc88 | mint                 | Like Wikipedia colour #3eb479 |
|   232 | #00cccc | darkturquoise      * | Like X11 colour #00ced1       |
|   236 | #44cccc | mediumturquoise    * | Like X11 colour #48d1cc       |
|   240 | #88cc88 | darkseagreen       * | Like X11 colour #8fbc8f       |
|   244 | #cccc88 | lightbeige           | Like Ford colour #d2d08e      |
|   248 | #88cccc | pearlaqua            | Like Wikipedia colour #88d8c0 |
|   252 | #cccccc | lightgrey          * | Like X11 colour #d3d3d3       |

 '*' means that a colour has the same name as one of the standard X11 colours
     and closely resembles it.
  
 '+' means that a colour has the same name as one of the standard X11 colours
     but is significantly darker.

-----------------------------------------------------------------------------
6   File formats
----------------

6.1 Compression format
----------------------

  Index and model data files for 'Chocks Away' are encoded using Gordon Key's
esoteric lossless compression algorithm.

  The first 4 bytes of a compressed file give the expected size of the data
when decompressed, as a 32 bit signed little-endian integer. Gordon Key's
file decompression module 'FDComp', which is presumably normative, rejects
input files where the top bit of the fourth byte is set (i.e. negative
values).

  Thereafter, the compressed data consists of tightly packed groups of 1, 8
or 9 bits without any padding between them or alignment with byte boundaries.
A decompressor must deal with two main types of directive: The first (store a
byte) consists of 1+8=9 bits and the second (copy previously-decompressed
data) consists of 1+9+8=18 or 1+9+9=19 bits.

The type of each directive is determined by whether its first bit is set:

0.   The next 8 bits of the input file give a literal byte value (0-255) to
   be written at the current output position.

     When attempting to compress input that contains few repeating patterns,
   the output may actually be larger than the input because each byte value
   is encoded using 9 rather than 8 bits.

1.   The next 9 bits of the input file give an offset (0-511) within the data
   already decompressed, relative to a point 512 bytes behind the current
   output position.

If the offset is greater than or equal to 256 (i.e. within the last 256
   bytes written) then the next 8 bits give the number of bytes (0-255) to be
   copied to the current output position. Otherwise, the number of bytes
   (0-511) to be copied is encoded using 9 bits.

If the read pointer is before the start of the output buffer then zeros
  should be written at the output position until it becomes valid again. This
  is a legitimate method of initialising areas of memory with zeros.

It isn't possible to replicate the whole of the preceding 512 bytes in
  one operation.

  The decompressors written by the Fourth Dimension and David O'Shea always
copy at least 1 byte from the source offset, even if the compressed bitstream
specified 0 as the number of bytes to be copied. A well-written compressor
should not insert directives to copy 0 bytes and no instances are known in
the wild. CBLibrary's decompressor treats directives to copy 0 bytes as
invalid input.

6.2 Index file
--------------

  When decompressed, an index file is interpreted as an array of up to 200
model data addresses. Each address occupies 4 bytes, stored in little-endian
byte order (i.e. least-significant bits first). This mechanism allows random
access to the model data for the game.

  The number of models available for use in a given map can be discovered
simply by dividing the index file size by 4.

6.3 Model data file
-------------------

  A model data file may contain invalid or unused data and therefore cannot
be interpreted (except perhaps using fuzzy logic) without using an index
file to find the start of the data for each model.

  In summary, the decompressed layout of the data for a single model is as
follows:

|  Offset | Size | Data
|---------|------|-----------------------
|       0 |   32 | Header
|      32 |  12v | Vertex definitions
|  32+12v |  16p | Primitive definitions

(where v is the no. of vertices and p is the no. of primitives)

6.3.1 Model header
------------------

  A simplified version of each model can be created from a subset of the
same data: a given number of primitives are always rendered but the
remainder are only rendered if the object is closer than a distance
specified for each model. The vertex and primitive counts for the simplified
model are stored separately from the counts for the complex model and should
always be smaller.

  If the primitive count for the selected detail level has the special value
of 255 then the first vertex of the model is rendered as a point and any
other vertices are ignored. It's unclear what use this is. Attempting to use
it would probably crash the map view.

  The vertex and primitive counts for the simplified model are ignored when
drawing the map.

  The near clip plane is an xz plane at a y position specified as part of
the model data. If any vertex of one of the primitives comprising the model
is closer than this plane (i.e. has a smaller y value) then that primitive
will be clipped.

  The last part of the per-model data controls plotting style. Lines can be
drawn one or two pixels thick and polygons can be drawn with or without a
blue or black outline. One word is overloaded to control both attributes:
models with outlined polygons must have thick lines and models with plain
polygons must have thin lines. Plotting style doesn't affect procedurally-
generated primitives.

  By default, thick lines and polygon outlines are disabled globally;
enabling them (by pressing '5' in Extra Missions) only affects models
configured to have those attributes in the first place. This attribute seems
to have been set in a rather arbitrary way. For example, patrol boats can
have outlined polygons and thick masts but gunboats cannot!

  Plot style is ignored when drawing the map: lines are thin and polygons
have no outlines.

Model header format:

|  Offset | Size | Data
|---------|------|---------------------------------------------------
|       0 |    4 | Model simplification distance
|       4 |    4 | Number of primitives -1, or 255 to draw 1st vertex
|       8 |    4 | Number of vertices -1
|      12 |    4 | Simplified number of primitives -1, or 255
|      16 |    4 | Simplified number of vertices -1
|      20 |    4 | Ignored
|      24 |    4 | Near clip plane distance
|      28 |    4 | Plot style (0, 1 or 2)

Plot styles:

 | Value | Line style | Polygon style       |
 |-------|------------|---------------------|
 |     0 | Thin       | No outline          |
 |     1 | Thick      | Black outline       |
 |     2 | Thick      | Royal azure outline |

6.3.2 Vertices
--------------

  An array of vertex definitions follows the per-model data. Each vertex
definition is 12 bytes long, comprising three coordinate values. Vertices
are not shared between models.

  The order in which vertices are defined is significant because their
position in the array is used to refer to them in primitive definitions. Any
vertices required by a simplified model should be defined before those only
required by the high-detail version of the same model, to minimize the
number of coordinates processed.

  It probably isn't practical to define more than 200 vertices in a single
model because higher vertex indices cannot be used in a model that is drawn
on the map (and some high indices trigger procedural generation when a
model is rendered in 3D).

  A left-handed coordinate system is used to specify vertex positions. Ground
level is the xy plane with z=0 and all negative z coordinate values are
above ground. Coordinates are stored as a signed two's complement little-
endian number occupying four bytes.

Vertex data format:

 | Offset | Size | Data
 |--------|------|---------------
 |      0 |    4 | X coordinate
 |      4 |    4 | Y coordinate
 |      8 |    4 | Z coordinate

6.3.3 Primitives
----------------

  An array of primitive definitions follows the vertex definitions. Each
primitive definition is 16 bytes long, half of which is used to store vertex
indices. Primitives are not shared between models.

  The order in which primitives are defined is significant for several
reasons:

1. Primitives belonging to both simplified and full versions of a model must
   precede those only required for the full model.

2. Primitives are rendered in the order they are defined regardless of their
   depth within a scene. Consequently every primitive should be defined
   before any others that could (partially) obscure it when projected into
   two dimensions. This is impossible for complex models, including some of
   the aircraft in the game.

3. Only the first primitive of most models is drawn on the map. This
   explains why square brown field boundaries are drawn as straight lines
   (annoying for navigation). A small list of exceptions is hardwired into
   the game to allow certain models to be drawn on the map without
   simplification ˜ for example cross-shaped aerodromes, which would
   otherwise appear to be missing a runway.

  Each vertex index occupies one byte. Vertices are indexed by their
position in the preceding vertex array, starting at 1. A primitive must have
at least two vertices and can have up to eight (an octagon). Primitives with
fewer than eight vertices are terminated by a zero byte. Polygons with
vertices specified in clockwise direction face the camera.

  Beyond a specified distance, a polygon is replaced with a line between its
first two vertices (as if the third byte of the definition were zero). This
threshold applies to the distance of the whole model from the camera, not
the distance of any individual vertex. A side-effect is to disable
procedural generation (e.g. stripes or dashes) and force the specified
colour and plotting style to be used.

  The simplified version of a model may be selected for an object receding
into the distance before its constituent polygons are simplified, or vice
versa. A simplified model's vertex count need not include any vertices not
required by polygons that are always simplified when that version of the
model is selected.

  The polygon simplification distance is ignored when drawing models on the
map: if a polygon is drawn at all then all of its edges are drawn.

Primitive data format:

  Offset  Size  Data
       0     1  1st vertex index
       1     1  2nd vertex index
       2     1  3rd vertex index (or 0 to terminate a line, or
                253..255 for patterns)
       3     1  4th vertex index (or 0 to terminate a triangle, or
                248..255 for patterns)
       4     1  5th vertex index (or 0 to terminate a quadrilateral)
       5     1  6th vertex index (or 0 to terminate a pentagon)
       6     1  7th vertex index (or 0 to terminate a hexagon)
       7     1  8th vertex index (or 0 to terminate a heptagon)
       8     1  Colour number
       9     3  Ignored
      12     4  Polygon simplification distance

6.3.4 Colour numbers
--------------------

  Colours are encoded using an additive RGB model with 4 bits per component.
However, the 2 least-significant bits of each component (the 'tint' bits)
must be equal for all three components.

| Bit | 7      | 6      | 5      | 4      | 3      | 2      | 1      | 0
|-----|--------|--------|--------|--------|--------|--------|--------|--------
|Role | B high | G high | G low  | R high | B low  | R low  | T high | T low

Red component:   (R high << 3) + (R low << 2) + (T high << 1) + T low

Green component: (G high << 3) + (G low << 2) + (T high << 1) + T low

Blue component:  (B high << 3) + (B low << 2) + (T high << 1) + T low

('<< n' means 'left-shift by n binary places'.)

For example, 42 (binary 00101010):
```
  B high 0, G high 0, G low 1, R high 0, B low 1, R low 0, T high 1, T low 0
  Red:   0010 = 2/15 = 0.13
  Green: 0110 = 6/15 = 0.4
  Blue:  0110 = 6/15 = 0.4
```
This is similar to the SVG/HTML/CSS colour named 'Teal' (0.0, 0.50, 0.50).

6.3.5 Procedural generation
---------------------------

  The game uses procedural generation to create parts of its 3D models. Road
and runway markings, battlements and trusses follow a regular pattern and
can be described more accurately and concisely by algorithm instead of
defining them by hand. Procedural generation is triggered by terminating a
line or triangle with a special vertex number instead of with zero. This
encoding scheme effectively limits the maximum number of vertices in a
polygon mesh to 252, unless care is taken to ensure that vertices numbered
253..255 never appear as the third vertex of a triangle. (A stricter
constraint applies to the fourth vertex of quadrilaterals and other complex
polygons.)

  Different special vertex numbers invoke different algorithms, using the
previous vertices as their input. Special lines can be drawn with different
thicknesses and numbers of dashes. Special triangles define parallelogram-
shaped regions to be filled with parallel lines (for railway sleepers),
zigzag lines (for trusses) or parallelograms (for stripes). Special
triangles are also used to draw evenly-spaced points (for landing lights).

  When drawing procedurally-generated primitives, the colour of the original
primitive is ignored in favour of a special colour. Similarly, the plotting
style of the model is ignored: polygons are drawn without outlines and line
thickness depends on the chosen algorithm. Line thickness and colour may
vary according to distance.

  Back-face culling is not applied to procedurally-generated polygons so
they effectively face both ways.

  Procedural generation is disabled when drawing models on the map. Special
primitives are drawn as ordinary triangles or lines, using the vertices and
colour of the original primitive.

Patterns triggered on the third vertex:

|  Vertex no. | Description       | Colour | No. of primitives |
|-------------|-------------------|--------|-------------------|
|         253 | Thin dashed line  | White  |                 8 |
|         254 | Thin dashed line  | White  |                16 |
|         255 | Thick dashed line | White  |                32 |

  Illustrations:

  253: Thin 8-dashed line
```
  1st                                                              2nd
   o____    ____    ____    ____    ____    ____    ____    ____    o
```

  254: Thin 16-dashed line
```
  1st                                                              2nd
   o__  __  __  __  __  __  __  __  __  __  __  __  __  __  __  __  o
```
  255: Thick 32-dashed line
```
  1st                                                              2nd
   o= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = o
```

Patterns triggered on the fourth vertex:

|  Vertex no. | Description                        | Colour    | No. of primitives |
|-------------|------------------------------------|-----------|-------------------|
|         248 | Dotted line (ignoring 3rd vertex)  | Orange    |                32 |
|         249 | Parallelograms                     | Dark grey |                16 |
|         250 | Thick parallel lines               | Peru      |                64 |
|         251 | Thin zigzag lines                  | Black     |                16 |
|         252 | Parallelograms if camera z <= -400 | Peridot   |                 8 |
|         253 | Parallelograms if camera z <= -400 | White     |                16 |
|         254 | Parallelograms                     | Peridot   |                 8 |
|         255 | Parallelograms                     | White     |                16 |


  Illustrations:
```
  248: Dotted line (ignoring 3rd vertex):
  1st                                                             2nd
   o. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .o

   o
  3rd
```

  249: 16 parallelograms:
```
  1st                                                             2nd
   o_   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _  o
   | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
   |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_|
   o
  3rd
```

  250: 64 thick parallel lines:
```
  2nd                                                             3rd
   o                                                               o
   ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
   ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
   o
  1st
```

  251: 16 thin zigzag lines:
```
  2nd                                                             3rd
   o       .       .       .       .       .       .       .       o
    \     / \     / \     / \     / \     / \     / \     / \     /
      \ /     \ /     \ /     \ /     \ /     \ /     \ /     \ /
   o   '       '       '       '       '       '       '       '
  1st
```

  252/254: 8 parallelograms:
```
  1st                                                             2nd
   o___     ___     ___     ___     ___     ___     ___     ___    o
   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|
   o
  3rd
```

  253/255 16 parallelograms:
```
  1st                                                             2nd
   o_   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _  o
   | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
   |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_|
   o
  3rd
```
-----------------------------------------------------------------------------
7  Program history
------------------

0.01 (02 Sep 2018)

  First public release.

0.02 (17 Nov 2018)

  Adapted to use CBUtilLib, StreamLib and GKeyLib instead of the monolithic
version of CBLibrary previously required.

0.03 (21 Apr 2020)

  Unnamed objects are now labelled "chocks_<n>" instead of "object_<n>" in
the output.

  Deleted a bad assertion that the error indicator for the output stream is
not set before beginning conversion. (The output stream pointer is null when
listing or summarizing object definitions.)

  Failure to close the output stream is now detected and treated like any
other error since data may have been lost.

  Corrected a description of the coordinate system (which is left-handed not
right-handed).

-----------------------------------------------------------------------------
8  Compiling the software
-------------------------

  Source code is only supplied for the command-line program. To compile
and link the code you will also require an ISO 9899:1999 standard 'C'
library and four of my own libraries: 3dObjLib, CBUtilLib, StreamLib and
GKeyLib. These are available separately from http://starfighter.acornarcade.com/mysite/

  Two make files are supplied:

1. 'Makefile' is intended for use with Acorn Make Utility (AMU) and the
   Norcroft C compiler supplied with the Acorn C/C++ Development Suite.

2. 'GMakefile' is intended for use with GNU Make and the GNU C Compiler.

  The APCS variant specified for the Norcroft compiler is 32 bit for
compatibility with ARMv5 and fpe2 for compatibility with older versions of
the floating point emulator. Generation of unaligned data loads/stores is
disabled for compatibility with ARMv6. When building the code for release,
it is linked with RISCOS Ltd's generic C library stubs ('StubsG').

  Before compiling the program for other platforms, rename the C source and
header files with .c and .h file extensions then move them out of their
respective subdirectories. The only platform-specific code is the
PATH_SEPARATOR macro definition in misc.h. This must be defined according to
the file path convention on the the target platform (e.g. '\' for DOS or
Windows).

-----------------------------------------------------------------------------
9  Licence and Disclaimer
-------------------------

  This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public Licence as published by the Free
Software Foundation; either version 2 of the Licence, or (at your option)
any later version.

  This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public Licence for
more details.

  You should have received a copy of the GNU General Public Licence along
with this program; if not, write to the Free Software Foundation, Inc.,
675 Mass Ave, Cambridge, MA 02139, USA.

-----------------------------------------------------------------------------
10  Credits
-----------

  ChocToObj was designed and programmed by Christopher Bazley.

  My information on the Wavefront Object file format came from Paul Bourke's
copy of the file format specifications for the Advanced Visualizer software
(Copyright 1995 Alias|Wavefront, Inc.):
  http://paulbourke.net/dataformats/obj/

  Some colour names were taken from a survey run by the web comic XKCD. The
data is in the public domain, available under a Creative Commons licence:
https://blog.xkcd.com/2010/05/03/color-survey-results/
Thanks to Keith McKillop for suggesting this source.

  The game Chocks Away is (C) 1990, The Fourth Dimension.

-----------------------------------------------------------------------------
11  Contact details
-------------------

  Feel free to contact me with any bug reports, suggestions or anything else.

  Email: mailto:cs99cjb@gmail.com

  WWW:   http://starfighter.acornarcade.com
