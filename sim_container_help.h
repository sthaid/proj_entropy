/*
Copyright (c) 2015 Steven Haid

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

char sim_container_help[] = "\
OVERVIEW\n\
--------\n\
\n\
This is a simulation of a gas expanding in a\n\
container. The gas is initially confined to a\n\
point located in the center of the container.\n\
When the simulation runs the gas expands from\n\
this central point to fill the container.\n\
\n\
The gas particles do not interact with each\n\
other, they only interact with the walls of the\n\
container.\n\
\n\
The ability to shrink and expand the container\n\
is provided. When the container is shrinking,\n\
the particles bounce off the container walls\n\
with a higher velocity. This higher reflection\n\
velocity is due to the inward velocity of the\n\
container walls when the container is shrinking.\n\
\n\
The color of each particle represents its speed.\n\
Slow moving particles are shown in purple, fast\n\
particles in red. The average speed of the\n\
particles is shown as a temperature value.\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP: Start and stop the simulation.\n\
\n\
SLOW/FAST: Control the speed of the simulation.\n\
\n\
SHRINK/RESTORE: Shrink and restore the container\n\
    size\n\
\n\
PB_REV/PB_FWD: Playback the simulation in the\n\
    reverse or forward directions\n\
\n\
RESET: Reset simulation to initial parameters.\n\
\n\
PARAMS: Supply new initial parameters.\n\
\n\
PARAMETERS\n\
----------\n\
\n\
PARTICLES: Number of particles\n\
\n\
SIM_WIDTH: Width of the container, in units of\n\
    particle diameter.\n\
\n\
LOW ENTROPY STOP\n\
----------------\n\
\n\
The simulation checks for the recreation of the\n\
initial low entropy state, and will enter the\n\
ENTROPY_STOP state when this is detected.\n\
\n\
WHAT TO OBSERVE\n\
---------------\n\
\n\
Increase of entropy when the simulation starts.\n\
\n\
There is a period of time after starting the\n\
simulation where the particles form a pattern\n\
that could be extrapolated back to the initial\n\
condition.\n\
\n\
Run the simulation with 4 particles, width set\n\
to 4, and at fastest speed.  After many cycles\n\
the simulation may detect low entropy, and\n\
automatically stop. You can then set the Run\n\
Speed to 1, and playback the simulation in\n\
reverse. This illustrates the statistical\n\
nature of entropy, and how entropy can very\n\
occasionally decrease.\n\
\n\
Shrink the container, and observe the\n\
temperature increase. Wait a bit to allow the\n\
temperature to equalize with the ambient\n\
temperature, of about 70 degrees. Then Restore\n\
the container, and notice the temperature fall.\n\
";
