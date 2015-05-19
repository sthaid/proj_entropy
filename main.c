#include "util.h"

#define SELECTION 1

#ifndef SELECTION
static char * choices[] = {
                "Expanding Gas in a Container",
                "Gravity Simulation",         
                "Our Expanding Universe",
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
        }
    }

    // XXX ABOUT / copyright

    // exit 
#if 0
    // XXX core dump here, argv corrupt
    PRINTF("XXX ARGV[0] %p\n", argv[0]);
#endif
    INFO("TERMINATING %s\n", argv[0]);
    sdl_terminate();
    return 0;
}

