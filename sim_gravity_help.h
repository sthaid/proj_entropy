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

// Note - max line length should not exceed 72 chars.
static char sim_gravity_help[] ="\
OVERVIEW\n\
--------\n\
\n\
This is a simulation of Newton's Law of Gravity.\n\
The motion of objects is simulated using the \n\
following equations:\n\
\n\
           M1 * M2\n\
    F = G --------\n\
            R^2\n\
\n\
    F = M * A\n\
\n\
The simulation is in 2 dimensions. Newton's \n\
Laws work in 2 dimensions; this is the case when\n\
the Z component of Force and the Z component of \n\
Initial Velocity are zero.\n\
\n\
The default center of the display is X,Y=(0,0);\n\
this can be changed by dragging the display. The\n\
center of the display can also be set to track\n\
one of the objects. To enable this mode click on\n\
the desired object. The object being tracked \n\
will be displayed in pink at the center of the \n\
display.\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP:  Start and stop the simulation.\n\
\n\
DT+/DT-: Change the time step of the simulation.\n\
    Smaller time steps provide a more accurate \n\
    simulation; however the simulation runs\n\
    more slowly.\n\
\n\
LOCAL/CLOUD: Select a config file either from \n\
    the Local system or the Cloud. These config\n\
    files describe the initial condition of the\n\
    simulation. \n\
\n\
PTHSTD/OFF/+/-: Adjust displayed path length.\n\
\n\
CONFIG FILES\n\
------------\n\
\n\
Config files provide the initial conditions for\n\
the simulation. The config files can be stored \n\
locally or in the cloud. \n\
\n\
The location of Local config files is:\n\
  Linux Version:    ./sim_gravity\n\
  Android Version:  embedded with the App Assets\n\
\n\
The location of Cloud config files is:\n\
  http://wikiscience101.sthaid.org/public/\n\
  sim_gravity/.\n\
If you are running the Linux Version, be sure \n\
you have curl installed.  Curl is used to access\n\
the Cloud config files.\n\
\n\
You can share your config files by uploading to\n\
the Cloud:\n\
  curl  -s --upload-file <your_config_file>  \n\
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
The solar_system simulation also contains Alpha\n\
Centauri. Find Alpha Centauri by zooming out, \n\
drag the display, and zoom in.\n\
\n\
When running the solar_system simulation, zoom \n\
in on the sun, and observe its small wobble. \n\
Observing the wobble of a star is one technique\n\
used by astronomers to detect extrasolar \n\
planets.\n\
\n\
The solar_system_and_rogue_star simulates the \n\
impact that a close approach of a rogue star \n\
would have on our solar system.\n\
\n\
Using the solar_system click on the Earth. The \n\
simulation display will track the Earth, that \n\
is the Earth will be displayed in the center.\n\
Observe the complicated paths the planets take \n\
from this perspective. The loops are called \n\
Epicycles or Retrograde Motion.\n\
\n\
Using gravity_boost try tracking the large \n\
object. Notice the similarities of the \n\
hyperbolic orbit of all the small objects  when\n\
viewed in the reference frame of the large \n\
obect.\n\
";
