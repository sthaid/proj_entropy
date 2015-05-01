#include "util.h"

// -----------------  MAIN  ----------------------------------------------

int32_t main(int32_t argc, char **argv)
{
    struct rlimit   rl;
    char          * choices[2] = {"CONTAINER", "GRAVITY" };
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
        sdl_display_choose_from_list("SELECT SIMULATION",
                                     choices, sizeof(choices)/sizeof(choices[0]),
                                     &selection);

        if (selection == -1) {
            break;
        }

        switch (selection) {
        case 0:
            sim_container();
            break;
        case 1:
            sim_gravity();
            break;
        }
    }

    // exit 
    // XXX core dump here, argv corrupt
    PRINTF("XXX ARGV[0] %p\n", argv[0]);
    INFO("TERMINATING %s\n", argv[0]);
    sdl_terminate();
    return 0;
}

