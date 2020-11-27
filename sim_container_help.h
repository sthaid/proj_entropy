// max line length = 44

char sim_container_help[] = "\
OVERVIEW\n\
--------\n\
\n\
This is a simulation of a gas expanding in a\n\
container. The gas is initially confined to\n\
a point located in the center of the\n\
container. When the simulation runs the gas\n\
expands from this central point to fill the\n\
container.\n\
\n\
The gas particles do not interact with each\n\
other, they only interact with the walls of\n\
the container.\n\
\n\
The ability to shrink and expand the\n\
container is provided. When the container is\n\
shrinking, the particles bounce off the\n\
container walls with a higher velocity.\n\
This higher reflection velocity is due to\n\
the inward velocity of the container walls\n\
when the container is shrinking.\n\
\n\
The color of each particle represents its\n\
speed. Slow moving particles are shown in\n\
purple, fast particles in red. The average\n\
speed of the particles is shown as a\n\
temperature value.\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP: Start and stop the simulation.\n\
\n\
SLOW/FAST: Control the speed of the\n\
    simulation.\n\
\n\
SHRINK/RESTORE: Shrink and restore the\n\
    container size\n\
\n\
PB_REV/PB_FWD: Playback the simulation in\n\
    the reverse or forward directions\n\
\n\
RESET: Reset simulation to initial\n\
     parameters.\n\
\n\
PARAMS: Supply new initial parameters.\n\
\n\
PARAMETERS\n\
----------\n\
\n\
PARTICLES: Number of particles\n\
\n\
SIM_WIDTH: Width of the container, in units\n\
    of particle diameter.\n\
\n\
LOW ENTROPY STOP\n\
----------------\n\
\n\
The simulation checks for the recreation of\n\
the initial low entropy state, and will\n\
enter the ENTROPY_STOP state when this is\n\
detected.\n\
\n\
WHAT TO OBSERVE\n\
---------------\n\
\n\
Increase of entropy when the simulation\n\
starts.\n\
\n\
There is a period of time after starting the\n\
simulation where the particles form a\n\
pattern that could be extrapolated back to\n\
the initial condition.\n\
\n\
Run the simulation with 4 particles, width\n\
set to 4, and at fastest speed.  After many\n\
cycles the simulation may detect low\n\
entropy, and automatically stop. You can\n\
then set the Run Speed to 1, and playback\n\
the simulation in reverse. This illustrates\n\
the statistical nature of entropy, and how\n\
entropy can very occasionally decrease.\n\
\n\
Shrink the container, and observe the\n\
temperature increase. Wait a bit to allow\n\
the temperature to equalize with the ambient\n\
temperature, of about 70 degrees. Then\n\
Restore the container, and notice the\n\
temperature fall.\n\
";
