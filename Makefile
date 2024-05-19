# Project:   ChocToObj

# Tools
CC = gcc
Link = gcc

# Toolflags:
CCCommonFlags = -c -Wall -Wextra -Wsign-compare -pedantic -std=c99 -MMD -MP
CCFlags = $(CCCommonFlags) -DNDEBUG -O3 -MF $*.d
CCDebugFlags = $(CCCommonFlags) -g -DDEBUG_OUTPUT -MF $*D.d
LinkCommonFlags = -o $@
LinkFlags = $(LinkCommonFlags) $(addprefix -l,$(ReleaseLibs))
LinkDebugFlags = $(LinkCommonFlags) $(addprefix -l,$(DebugLibs))

include MakeCommon

DebugObjectsChoc = $(addsuffix .debug,$(ObjectList))
ReleaseObjectsChoc = $(addsuffix .o,$(ObjectList))
DebugLibs = CBUtildbg Streamdbg GKeydbg 3dObjdbg m
ReleaseLibs = CBUtil Stream GKey 3dObj m

# Final targets:
all: ChocToObj ChocToObjD 

ChocToObj: $(ReleaseObjectsChoc)
	$(Link) $(ReleaseObjectsChoc) $(LinkFlags)

ChocToObjD: $(DebugObjectsChoc)
	$(Link) $(DebugObjectsChoc) $(LinkDebugFlags)


# User-editable dependencies:
.SUFFIXES: .o .c .debug
.c.debug:
	$(CC) $(CCDebugFlags) -o $@ $<
.c.o:
	$(CC) $(CCFlags) -o $@ $<

# Static dependencies:

# Dynamic dependencies:
# These files are generated during compilation to track C header #includes.
# It's not an error if they don't exist.
-include $(addsuffix .d,$(ObjectList))
-include $(addsuffix D.d,$(ObjectList))
