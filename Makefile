TARGETS = entropy

CC = gcc
CFLAGS = -c -g -O2 -pthread -funsigned-char -Wall $(shell sdl2-config --cflags)

ENTROPY_OBJS = entropy.o util.o 

#
# build rules
#

all: $(TARGETS)

entropy: $(ENTROPY_OBJS) 
	$(CC) -pthread -lrt -lSDL2 -lSDL2_ttf -lSDL2_mixer -o $@ $(ENTROPY_OBJS)

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
