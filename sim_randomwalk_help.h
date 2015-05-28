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

// Note max line length is 48 chars.

static char sim_randomwalk_help[] = "\
OVERVIEW\n\
--------\n\
\n\
Simulation of Random Walk in 2 Dimensions. Each\n\
walker randomly chooses to move 1 unit (take\n\
one step) in either the positive or negative X\n\
or Y direction.\n\
\n\
The average and expected distance from the\n\
origin is displayed. The expected distance from\n\
the origin is given by the following equation:\n\
\n\
                    GAMMA((D + 1) / 2)\n\
  SQRT(2 * N / D) * ------------------\n\
                      GAMMA(D / 2)\n\
\n\
  where N: number of steps\n\
        D: number of dimensions\n\
\n\
  http://math.stackexchange.com/\n\
  questions/103142/expected-value-of-random-walk\n\
\n\
CONTROLS\n\
--------\n\
\n\
RUN/STOP:  Start and stop the simulation.\n\
\n\
SLOW/FAST: Control the speed of the simulation.\n\
\n\
RESET:     Reset simulation to initial params.\n\
\n\
PARAMS:    Supply new initial parameters, and\n\
           reset to the new params\n\
\n\
PARAMETERS\n\
----------\n\
\n\
N_WLK: Number of random walkers.\n\
\n\
WHAT TO OBSERVE\n\
---------------\n\
\n\
Observe the Ratio of the Actual and Expected\n\
Distance.\n\
\n\
Configure with a small number of walkers, and\n\
observe the individual paths.\n\
";
