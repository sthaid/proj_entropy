#
# NOTE: To use SDL2_GFX
#   1) add the following to OBJS
#         SDL2_GFX_OBJS = sdl2-gfx/src/SDL2_framerate.o \
#                         sdl2-gfx/src/SDL2_gfxPrimitives.o \
#                         sdl2-gfx/src/SDL2_imageFilter.o \
#                         sdl2-gfx/src/SDL2_rotozoom.o
#   2) add the following to CFLAGS
#         -I ./sdl2-gfx/include  -DUSE_MMX
#   3) add include to source
#         #include <SDL2_gfxPrimitives.h>
#

TARGETS = entropy

CC = gcc
CFLAGS = -c -g -O2 -pthread -fsigned-char -Wall \
         $(shell sdl2-config --cflags) 

OBJS = main.o sim_container.o sim_gravity.o sim_universe.o sim_randomwalk.o sim_gameoflife.o util.o 

#
# build rules
#

all: $(TARGETS)

entropy: $(OBJS) 
	echo "char *version = \"`git log -1 --format=%h`\";" > version.c
	$(CC) -pthread -o $@ $(OBJS) version.c -lrt -lSDL2 -lSDL2_ttf -lm -lpng

#
# clean rule
#

clean:
	rm -f $(TARGETS) $(OBJS)

#
# compile rules
#

main.o: main.c util.h about.h
util.o: util.c util.h
sim_container.o: sim_container.c util.h sim_container_help.h
sim_gravity.o: sim_gravity.c util.h sim_gravity_help.h
sim_universe.o: sim_universe.c util.h sim_universe_help.h
sim_randomwalk.o: sim_randomwalk.c util.h sim_randomwalk_help.h
sim_gameoflife.o: sim_gameoflife.c util.h sim_gameoflife_help.h

