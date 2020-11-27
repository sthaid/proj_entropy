#include "util.h"
#include "about.h"

//
// defines
//

#define DEFAULT_WIDTH   1280
#define DEFAULT_HEIGHT  800
#define MIN_WIDTH       600
#define MAX_WIDTH       4096
#define MIN_HEIGHT      400
#define MAX_HEIGHT      4096

//
// variables
//

static char * choices[] = {
                "Expanding Gas in a Container",
                "Gravity Simulation",         
                "Our Expanding Universe",
                "Random Walk",
                "About",
                        };

//
// prototypes
//

#ifndef ANDROID
static void usage(void);
#endif

// -----------------  MAIN  ----------------------------------------------

int32_t main(int32_t argc, char **argv)
{
    struct rlimit   rl;
    int32_t         selection;
    int32_t         width  = DEFAULT_WIDTH;
    int32_t         height = DEFAULT_HEIGHT;

    // init core dumps
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    // init logging 
    logmsg_init("stderr");
    INFO("STARTING %s\n", argv[0]);

#ifndef ANDROID
    // parse options
    while (true) {
        char opt_char = getopt(argc, argv, "g:hv");
        if (opt_char == -1) {
            break;
        }
        switch (opt_char) {
        case 'g':
            if (sscanf(optarg, "%dx%d", &width, &height) != 2 ||
                width < MIN_WIDTH || width > MAX_WIDTH ||
                height < MIN_HEIGHT || height > MAX_HEIGHT)
            {
                usage();
                return 1;
            }
            break;
        case 'h':
            usage();
            return 0;
            break;
        case 'v':
            printf("Version %s\n", VERSION_STR);
            return 0;
        default:
            return 1;
        }
    }

    // print options
    INFO("args: width,height = %dx%d\n", width, height);
#endif

    // init sdl
    sdl_init(width, height);

    // processing
    while (!sdl_quit) {
        sdl_display_choose_from_list("-- Choose A Simulation --",
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
        case 2:
            sim_universe();
            break;
        case 3:
            sim_randomwalk();
            break;
        case 4:
            sdl_display_text(about);
            break;
        }
    }

    // exit 
    INFO("TERMINATING %s\n", argv[0]);
    sdl_terminate();
    return 0;
}

#ifndef ANDROID
static void usage(void)
{
    printf("\
\n\
entropy: [-g WxH] [-h] [-v]\n\
  -g: geometry, for example -g 1600x900, default is %dx%d\n\
  -h: help\n\
  -v: version\n\
\n\
Refer to Help selection provided by each simulation.\n\
Also refer to http://wikiscience101.sthaid.org/index.php?title=Entropy_App.\n\
\n\
Use Ctrl-P for screen capture, only supported on the Linux version.\n\
\n\
",
        DEFAULT_WIDTH, DEFAULT_HEIGHT);
}
#endif
