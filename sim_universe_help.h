// max line length = 44 

static char sim_universe_help[] = "\
OVERVIEW\n\
--------\n\
\n\
This is a simulation of our expanding \n\
universe.  The simulated universe has the \n\
following characteristics:\n\
- it is comprised of non interacting \n\
  particles\n\
- each particle is given an initial random \n\
  velocity, which does not change\n\
- the radius of the universe increases with \n\
  time, this causes an additional apparent \n\
  velocity which increases with distance.\n\
\n\
The vantage point is the center of the \n\
display.  The color of a particle indicates\n\
its apparent velocity with respect to the \n\
vantage point. The apparent velocity is the\n\
sum of the particle's actual velocity and \n\
apparent velocity due to the expansion.\n\
\n\
Particles shown in:\n\
- blue  - apparent velocity is toward the \n\
          vantage point\n\
- green - small apparent velocity\n\
- red   - apparent velocity away from the \n\
          vantage point, near the speed of \n\
          light\n\
- purple- these particles are not visible \n\
          from the vantage point, because \n\
          their apparent velocity exceed \n\
          the speed of light\n\
\n\
Note - All vantage points in our universe \n\
are equivalent. This means that the universe\n\
appears generally the same from every \n\
vantage point.\n\
\n\
Note - The particles shown in purple have \n\
apparent recession velocities, from the \n\
vantage point in the center, that exceed the\n\
speed of light. This does not violate \n\
Special Relativity because their large \n\
velocities are due to the expansion of \n\
space itself.\n\
\n\
RADIUS OF THE UNIVERSE\n\
----------------------\n\
\n\
The simulation uses the following formula to\n\
calculate the radius of the universe. This \n\
formula was chosen for this simulation \n\
because its behavior is similar to accepted\n\
theories of the expansion of our universe:\n\
- very rapid increase of radius at low \n\
  values of time (inflation)\n\
- rapid increase of radius at high time \n\
  values \n\
\n\
  Radius = K1*sqrt(T)  +  K2*(2^(T/K3) - 1)\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP:  Start and stop the simulation.\n\
\n\
SUS/RES:   Suspend and resume the expansion\n\
           of the universe. The motion of \n\
           the particles is not affected.\n\
\n\
SLOW/FAST: Control the speed of the \n\
           simulation.\n\
\n\
RESET:     Reset simulation to initial \n\
           params.\n\
\n\
PARAMS:    Supply new initial parameters, \n\
           and reset to the new params\n\
\n\
PARAMETERS\n\
----------\n\
\n\
N_PART: Number of particles in the \n\
    simulation.  The simulation runs slower\n\
    with larger number of particles.\n\
\n\
START:  Initial time. The default start time\n\
    is 0.000001 billion years, or 1000 \n\
    years after the big bang.\n\
\n\
AVGSPD: The average speed of the particles.\n\
\n\
WHAT TO OBSERVE\n\
---------------\n\
\n\
Rapid expansion during the early universe.\n\
\n\
much of the universe lies beyond what we can\n\
see from our vantage point.\n\
\n\
As the universe ages, the size of the \n\
observable universe shrinks towards zero.\n\
";
