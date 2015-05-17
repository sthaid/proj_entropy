char sim_container_help[] = "\
OVERVIEW\n\
--------\n\
\n\
This is a simulation of a gas expanding in a container. The gas is \n\
initially confined to a point located in the center of the container. \n\
When the simulation runs the gas expands from this central point to \n\
fill the container.\n\
\n\
The particles do not interact with each other, they only interact with \n\
the walls of the container.\n\
\n\
The ability to shrink and expand the container is provided. When the\n\
container is shrinking, the particles bounce off the container walls \n\
with a higher velocity. This higher reflection velocity is due to the \n\
inward velocity of the container walls when the container is shrinking.\n\
\n\
The color of each particle represents its speed. Slow moving particles \n\
are shown in purple, fast particles in red. The average speed of the \n\
particles is shown as a temperature value.\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP: Start and stop the simulation.\n\
\n\
SLOW/FAST: Control the speed of the simulation.\n\
\n\
SHRINK/RESTORE: Shrink and restore the container size\n\
\n\
PB_REV/PB_FWD: Playback the simulation in the reverse or forward \n\
        directions\n\
\n\
RESET:  Reset simulation to initial parameters.\n\
\n\
PARAMS: Supply new initial parameters.\n\
\n\
PARAMETERS\n\
----------\n\
\n\
PARTICLES: Number of particles\n\
\n\
SIM_WIDTH: Width of the container, in units of particle diameter.\n\
\n\
LOW ENTROPY STOP\n\
----------------\n\
\n\
The simulation checks for the recreation of the initial low entropy \n\
state, and will enter the ENTROPY_STOP state when this is detected.\n\
\n\
WHAT TO OBSERVE\n\
---------------\n\
\n\
Increase of entropy when the simulation starts.\n\
\n\
There is a period of time after starting the simulation where the \n\
particles form a pattern that could be extrapolated back to the initial\n\
condition. \n\
\n\
Run the simulation with 4 particles, width set to 4, and at fastest \n\
speed.  After many cycles the simulation may detect low entropy, and \n\
automatically stop. You can then set the Run Speed to 1, and playback \n\
the simulation in reverse. This illustrates the statistical nature of\n\
entropy, and how entropy can very occasionally decrease.\n\
\n\
Shrink the container, and observe the temperature increase. Wait a bit \n\
to allow the temperature to equalize with the ambient temperature, of \n\
about 70 degrees. Then Restore the container, and notice the \n\
temperature fall.\n\
";
