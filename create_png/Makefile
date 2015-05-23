TARGETS = create_png
CC = gcc
CFLAGS = -c -g -O2 -pthread -fsigned-char -Wall \
         $(shell sdl2-config --cflags) 

#
# build rules
#

all: $(TARGETS)

create_png: create_png.o
	$(CC) -lSDL2 -lSDL2_ttf -lpng -o $@ create_png.o

#
# clean rule
#

clean:
	rm -f $(TARGETS) *.o
