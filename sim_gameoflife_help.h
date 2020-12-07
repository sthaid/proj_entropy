// Note max line length is 44 chars.

static char sim_gameoflife_help[] = "\
OVERVIEW\n\
--------\n\
\n\
Refer to \"Conway\'s Game of Life\" on Wikipedia.\n\
\n\
Simulation of Life on a two dimensional grid\n\
of cells. Each cell is in either the Live \n\
state or the Dead state.\n\
\n\
Each Cell interact with its 8 neighbors, \n\
following these rules:\n\
\n\
1) Any live cell with fewer than two live \n\
   neighbors dies, as if by underpopulation.\n\
2) Any live cell with two or three live \n\
   neighbors lives on to the next generation.\n\
3) Any live cell with more than three live \n\
   neighbors dies, as if by overpopulation.\n\
4) Any dead cell with exactly three live \n\
   neighbors becomes a live cell, as if by\n\
   reproduction.\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/PAUSE/CONT/RESTART: Start and stop.\n\
\n\
RESET: Set speed, zoom, and center to default.\n\
\n\
SLOW/FAST: Adjust speed.\n\
\n\
ZOOM+/-: Zoom in and out.\n\
\n\
SLCT+/-: Choose starting pattern.\n\
\n\
To pan, touch and drag.\n\
";
