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
This is a simulation of Newton's Law of Gravity. The motion of objects\n\
is simulated using the following equations:\n\
\n\
           M1 * M2\n\
    F = G --------\n\
            R^2\n\
\n\
    F = M * A\n\
\n\
The simulation is in 2 dimensions. Newton's Laws work in 2 dimensions;\n\
this is the case when the Z component of Force and the Z component of \n\
Initial Velocity are zero.\n\
\n\
The default center of the display is X,Y=(0,0); this can be changed by \n\
dragging the display. The center of the display can also be set to track\n\
one of the objects. To enable this mode click on the desired object. The\n\
object being tracked will be displayed in pink at the center of the \n\
display.\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP:  Start and stop the simulation.\n\
\n\
DT+/DT-: Change the time step of the simulation. Smaller time steps\n\
    provide a more accurate simulation; however the simulation runs\n\
    more slowly.\n\
\n\
LOCAL/CLOUD: Select a config file either from the Local system or\n\
    the Cloud. These config files describe the initial condition\n\
    of the simulation. \n\
\n\
PATH_DFLT/OFF/+/-: Adjust the displayed path length.\n\
\n\
CONFIG FILES\n\
------------\n\
\n\
Config files provide the initial conditions for the simulation. The \n\
config files can be stored locally or in the cloud. \n\
\n\
The location of Local config files is:\n\
  Linux Version:    ./sim_gravity\n\
  Android Version:  embedded with the App (in Assets).\n\
\n\
The location of Cloud config files is:\n\
  http://wikiscience101.sthaid.org/public/sim_gravity/.\n\
If you are running the Linux Version, be sure you have curl installed.\n\
Curl is used to access the Cloud config files.\n\
\n\
You can share your config files by uploading to the Cloud:\n\
  curl  -s --upload-file <your_config_file> \\ \n\
       http://wikiscience101.sthaid.org/public/sim_gravity/\n\
\n\
The config file format is documented in the solar_system file.\n\
\n\
WHAT TO OBSERVE\n\
---------------\n\
\n\
Try each config file. Notice the 2 body simulations are stable, and \n\
the 3 body simulation is chaotic.\n\
\n\
The solar_system simulation also contains Alpha Centauri. Find Alpha \n\
Centauri by zooming out, drag display, zoom in.\n\
\n\
The solar_system_and_rogue_star simulates the impact that a close \n\
approach of a rogue star would have on our solar system.\n\
\n\
Using the solar_system click on the Earth. The simulation display will\n\
track the Earth, that is the Earth will be displayed in the center.\n\
Observe the complicated paths the planets take from this perspective.\n\
The loops are called Epicycles or Retrograde Motion.\n\
\n\
Using gravity_boost try tracking the large object. Notice the \n\
similarities of the hyperbolic orbit of the all the small objects  \n\
when viewed in the reference frame of the large obect.\n\
";
