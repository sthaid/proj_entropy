// max line length = 44

static char sim_gravity_help[] ="\
OVERVIEW\n\
--------\n\
\n\
This is a simulation of Newton's Law of \n\
Gravity.  The motion of objects is simulated\n\
using the following equations:\n\
\n\
           M1 * M2\n\
    F = G --------\n\
            R^2\n\
\n\
    F = M * A\n\
\n\
The simulation is in 2 dimensions. Newton's\n\
Laws work in 2 dimensions; this is the case\n\
when the Z component of Force and the Z \n\
component of Initial Velocity are zero.\n\
\n\
The default center of the display is \n\
X,Y=(0,0); this can be changed by dragging \n\
the display. The center of the display can \n\
also be set to track one of the objects. To\n\
enable this mode click on the desired \n\
object. The object being tracked will be \n\
displayed in pink at the center of the \n\
display.\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP:  Start and stop the simulation.\n\
\n\
DT+/DT-: Change the time step of the \n\
    simulation.  Smaller time steps provide\n\
    a more accurate simulation; however the\n\
    simulation runs more slowly.\n\
\n\
SELECT: Select a config file, the config file\n\
    describe the initial condition of the\n\
    simulation. \n\
\n\
PTHSTD/OFF/+/-: Adjust displayed path len.\n\
\n\
CONFIG FILES\n\
------------\n\
\n\
Config files provide the initial conditions\n\
for the simulation.\n\
\n\
The config files are in:\n\
  Linux Version:    ./sim_gravity\n\
  Android Version:  embedded in the App \n\
\n\
The config file format is documented in the \n\
solar_system file.\n\
\n\
WHAT TO OBSERVE\n\
---------------\n\
\n\
Try each config file. Notice the 2 body \n\
simulations are stable, and the 3 body \n\
simulation is chaotic.\n\
\n\
The solar_system simulation also contains \n\
Alpha Centauri. Find Alpha Centauri by \n\
zooming out, drag the display, and zoom in.\n\
\n\
When running the solar_system simulation, \n\
zoom in on the sun, and observe its small \n\
wobble.  Observing the wobble of a star is \n\
one technique used by astronomers to detect \n\
extrasolar planets.\n\
\n\
The solar_system_and_rogue_star simulates \n\
the impact that a close approach of a rogue\n\
star would have on our solar system.\n\
\n\
Using the solar_system click on the Earth. \n\
The simulation display will track the Earth,\n\
that is the Earth will be displayed in the \n\
center. Observe the complicated paths the \n\
planets take from this perspective. The \n\
loops are called Epicycles or Retrograde \n\
Motion.\n\
\n\
Using gravity_boost try tracking the large \n\
object. Notice the similarities of the \n\
hyperbolic orbit of all the small objects \n\
when viewed in the reference frame of the \n\
large object.\n\
";
