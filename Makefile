TARGETS = entropy

CC = gcc
CFLAGS = -c -g -O2 -pthread -fsigned-char -Wall \
         $(shell sdl2-config --cflags) \
         -I ./sdl2-gfx/include  -DUSE_MMX

SDL2_GFX_OBJS = sdl2-gfx/src/SDL2_framerate.o \
                sdl2-gfx/src/SDL2_gfxPrimitives.o \
                sdl2-gfx/src/SDL2_imageFilter.o \
                sdl2-gfx/src/SDL2_rotozoom.o
ENTROPY_OBJS = entropy.o util.o 
OBJS = $(ENTROPY_OBJS) $(SDL2_GFX_OBJS)

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
	rm -f $(TARGETS) *.o

#
# compile rules
#

entropy.o: entropy.c util.h
util.o: util.c util.h button_sound.h

.c.o: 
	$(CC) $(CFLAGS) $< -o $@
