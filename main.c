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

#include "util.h"
#include "about.h"

// #define SELECTION 1

#ifndef SELECTION
static char * choices[] = {
                "Expanding Gas in a Container",
                "Gravity Simulation",         
                "Our Expanding Universe",
                "About",
                        };
#endif

// -----------------  MAIN  ----------------------------------------------

int32_t main(int32_t argc, char **argv)
{
    struct rlimit   rl;
    int32_t         selection;

    // init core dumps
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    // init logging 
    logmsg_init("stderr");
    INFO("STARTING %s\n", argv[0]);

    // init sdl
    sdl_init(1280,800);

    // processing
    while (!sdl_quit) {
#ifndef SELECTION
        sdl_display_choose_from_list("-- Choose A Simulation --",
                                     choices, sizeof(choices)/sizeof(choices[0]),
                                     &selection);

        if (selection == -1) {
            break;
        }
#else
        selection = SELECTION;
#endif

        switch (selection) {
        case 0:
            sim_container();
            break;
        case 1:
            sim_gravity();
            break;
        case 2:
            sim_universe();
            break;
        case 3:
            sdl_display_text(about);
            break;
        }
    }

    // exit 
    INFO("TERMINATING %s\n", argv[0]);
    sdl_terminate();
    return 0;
}

