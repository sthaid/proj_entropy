#include "util.h"

#include <SDL2_gfxPrimitives.h>

//
// CONFIG Support
//

#define CONFIG_WRITE() \
    do { \
        config_write(config_path, config, config_version); \
    } while (0)

#define CONFIG_DEBUG (config[0].value[0])    // N, Y

char          config_path[PATH_MAX];
const int32_t config_version = 1;
config_t      config[] = { { "debug",     "Y"         },
                           { "",          ""          } };

#ifndef ANDROID
    #define GET_CONFIG_DIR() getenv("HOME")
#else
    #define GET_CONFIG_DIR() SDL_AndroidGetInternalStoragePath();
#endif

//
// defines
//

#define EVENT_ONE  1  //xxx

//
// typedefs
//

//
// variables
//

#if 0 //xxx later
int32_t  param_radius        = DEFAULT_PARAM_RADIUS;
int32_t  param_max_particles = DEFAULT_PARAM_MAX_PARTICLES;
bool reset               = true;
int32_t  mode                = MODE_STOPPED;
int32_t  speed               = SPEED_NORMAL;
int32_t  event               = EVNET_NONE;
int32_t  force_field         = FORCE_FIELD_OFF;
#endif

//
// prototypes
//

void init(char * progname);
void terminate(char * progname);
void main_loop(void);

// -----------------  MAIN  ----------------------------------------------

int32_t main(int32_t argc, char **argv)
{
    // init
    init(argv[0]);

    // processing
    while (!sdl_quit_event) {
        main_loop();
        //usleep(10000);
    }

    // done
    terminate(argv[0]);
    return 0;
}

void init(char *progname)
{
    struct rlimit    rl;
    int32_t          ret;
    const char     * config_dir;

    // set resource limti to allow core dumps
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    ret = setrlimit(RLIMIT_CORE, &rl);
    if (ret < 0) {
        WARN("setrlimit for core dump, %s\n", strerror(errno));
    }

    // read entropy config
    config_dir = GET_CONFIG_DIR();
    if (config_dir == NULL) {
        FATAL("failed to get config_dir\n");
    }
    sprintf(config_path, "%s/.entropy_config", config_dir);
    if (config_read(config_path, config, config_version) < 0) {
        FATAL("config_read failed for %s\n", config_path);
    }

    // init logging
    logmsg_init(CONFIG_DEBUG == 'Y' ? "stderr" : "none");
    INFO("STARTING %s\n", progname);

    // sdl init
    sdl_init();
}

void terminate(char * progname)
{
    INFO("TERMINATING %s\n", progname);
    sdl_terminate();
}

// -----------------  UPDATE DISPLAY  -----------------------------------------------------------

// XXX main body of work in here
void main_loop(void)
{
    char str[100];
    static int32_t count;
    static int32_t event_one_count;
    SDL_Rect    ctlpane;
    char event_one_str[500];

    SDL_INIT_PANE(ctlpane,
                  0, 0,     // x, y
                  sdl_win_width, sdl_win_height);     // w, h
    sdl_event_init();

    // clear window
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);

    // display stuff
    sprintf(str, "HELLO %d %d", count++, event_one_count);
    sdl_render_text_font1(&ctlpane, 0, 0, str, EVENT_ONE);

    sprintf(str, "width=%d height=%d min=%d",
        sdl_win_width, sdl_win_height, sdl_win_minimized);
    sdl_render_text_font0(&ctlpane, 5, 0, str, SDL_EVENT_NONE);


#if 0
    uint32_t color = SDL_MapRGB(SDL_PIXELFORMAT_RGB24, 200, 200, 200);
    filledCircleColor(sdl_renderer, 100, 100, 75, color);
#endif

    #define MAX_X 10000
    #define MAX_Y 10000

    int dir;
    static int dx[8] = {   0, 71, 100,  71,    0, -71, -100, -71 };
    static int dy[8] = { 100, 71,   0, -71, -100, -71,    0,  71 };
    static int x = MAX_X / 2;
    static int y = MAX_Y / 2;

    dir = random() % 8;
    x = x + dx[dir];
    y = y + dy[dir];
    if (x < 0 || x > MAX_X)  {
        x = x - 2 * dx[dir];
    }
    if (y < 0 || y > MAX_X)  {
        y = y - 2 * dy[dir];
    }

   filledCircleRGBA(sdl_renderer, x*sdl_win_width/MAX_X, y*sdl_win_height/MAX_Y, 10, 255, 255, 255, 255);

    // present the display
    SDL_RenderPresent(sdl_renderer);

    // poll for events
    sdl_poll_event();

    // handle events
    if (sdl_event != SDL_EVENT_NONE) {
        if (sdl_event == EVENT_ONE) {
            event_one_count++;
            sdl_get_string("PROMPT_STR", event_one_str, sizeof(event_one_str));
            PRINTF("GOT STRING '%s'\n", event_one_str);
        } else {
            ERROR("invalid event %d\n", sdl_event);
        }
        sdl_event = SDL_EVENT_NONE;
    }
}

