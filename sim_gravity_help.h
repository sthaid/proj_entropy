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
LOCAL/CLOUD: Select a config file either \n\
    from the Local system or the Cloud. \n\
    These config files describe the initial\n\
    condition of the simulation. \n\
\n\
PTHSTD/OFF/+/-: Adjust displayed path len.\n\
\n\
CONFIG FILES\n\
------------\n\
\n\
Config files provide the initial conditions\n\
for the simulation. The config files can be\n\
stored locally or in the cloud. \n\
\n\
The location of Local config files is:\n\
  Linux Version:    ./sim_gravity\n\
  Android Version:  embedded with the App \n\
\n\
The location of Cloud config files is:\n\
  http://wikiscience101.sthaid.org/public/\n\
  sim_gravity/.\n\
If you are running the Linux Version, be \n\
sure you have curl installed.  Curl is used \n\
to access the Cloud config files.\n\
\n\
You can share your config files by uploading\n\
to the Cloud:\n\
  curl  -s --upload-file <your_config_file> \n\
    http://wikiscience101.sthaid.org/public/\n\
    sim_gravity/\n\
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
