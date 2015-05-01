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

OBJS = main.o sim_container.o sim_gravity.o util.o 

#
# build rules
#

all: $(TARGETS)

entropy: $(OBJS) 
	$(CC) -pthread -lrt -lSDL2 -lSDL2_ttf -lSDL2_mixer -lm -o $@ $(OBJS)

#
# clean rule
#

clean:
	rm -f $(TARGETS) $(OBJS)

#
# compile rules
#

main.o: main.c util.h
util.o: util.c util.h button_sound.h
sim_container.o: sim_container.c util.h
sim_gravity.o: sim_gravity.c util.h

