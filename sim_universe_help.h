// XXX review this
// XXX line widhts
static char sim_universe_help[] = "\
OVERVIEW\n\
--------\n\
\n\
This is a simulation of our expanding universe. The simulated universe \n\
has the following characteristics:\n\
- it is comprised of non interacting particles\n\
- each particle is given an initial random velocity, which does not \n\
  change\n\
- the radius of the universe increases with time, this causes \n\
  an additional apparent velocity which increases with distance.\n\
\n\
The vantage point is the center of the display. The color of a particle\n\
indicates its apparent velocity with respect to the vantage point. The\n\
apparent velocity is the sum of the particle's actual velocity and \n\
apparent velocity due to the expansion.\n\
\n\
Particles shown in:\n\
- blue  - apparent velocity is toward the vantage point\n\
- green - no apparent velocity\n\
- red   - apparent velocity away from the vantage point, near the speed\n\
           of light\n\
- purple- these particles are not visible from the vantage point, \n\
          because their apparent velocity exceed the speed of light\n\
\n\
Note - All vantage points in the universe are equivalent. This means \n\
that the universe appears generally the same from every vantage point.\n\
\n\
Note - The particles shown in purple have apparent recession velocities\n\
from the vantage point in the center that exceed the speed of light. \n\
This does not violate Special Relativity because their large velocities\n\
are due to the expansion of space itself.\n\
\n\
RADIUS OF THE UNIVERSE\n\
----------------------\n\
\n\
The simulation uses the following formula to calculate the radius of the\n\
universe. This formula was chosen because it behavior which is similar \n\
to some accepted theories of our universe:\n\
- very rapid increase of radius at low values of time (inflation)\n\
- rapid increase of radius at high values of time\n\
\n\
Radius = K1 * sqrt(T)  +  K2 * (2^(T/K3) - 1)\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP:  Start and stop the simulation.\n\
\n\
SUS/RES:   Suspend and resume the expansion of the universe. The motion\n\
           of the particles is not affected.\n\
\n\
SLOW/FAST: Control the speed of the simulation.\n\
\n\
RESET:     Reset simulation to initial parameters.\n\
\n\
PARAMS:    Supply new initial parameters, and reset the simulation to \n\
           the new parameters.\n\
\n\
PARAMETERS\n\
----------\n\
\n\
N_PART: Number of particles in the simulation. The simulation runs \n\
        slower with larger number of particles.\n\
\n\
START:  Initial time. The default start time is 0.000001 billion years,\n\
        or 1000 years after the big bang.\n\
\n\
AVGSPD: The average speed of the particles.\n\
\n\
WHAT TO OBSERVE\n\
---------------\n\
\n\
Rapid expansion during the early universe.\n\
\n\
Much of the universe lies beyond what we can see from our vantage point.\n\
\n\
As the universe ages, the size of the observable universe shrinks \n\
towards zero.\n\
";
