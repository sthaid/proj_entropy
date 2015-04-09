#include "util.h"

//
// defines
//

#define CONFIG_DEBUG (config.ent[0].value[0])    // N, Y

#define MAX_MODE_TBL (sizeof(mode_tbl) / sizeof(mode_tbl[0]))

//
// typedefs
//

typedef struct {
    char * select_menu_name;
    bool (*sim)(void);
} mode_tbl_t;

//
// variables
//

static int32_t  mode;

static bool     mode_select  = false;  // XXX true
static config_t config       = { 1, 
                                 { { "debug",     "Y"         },
                                   { "",          ""          } } };
static mode_tbl_t mode_tbl[] = { { "CONTAINER", sim_container }, };

//
// prototypes
//

void display_handler_mode_select(void);

// -----------------  MAIN  ----------------------------------------------

int32_t main(int32_t argc, char **argv)
{
    struct rlimit rl;

    // init core dumps
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    // init config
    config_read(".entropy_main", &config);

    // init logging
    logmsg_init(CONFIG_DEBUG == 'Y' ? "stderr" : "none");
    INFO("STARTING %s\n", argv[0]);

    // init sdl
    sdl_init(1280,800);

    // processing
    while (!sdl_quit) {
        if (mode_select) {
            display_handler_mode_select();
        } else {
            // printf("CALLING MODE HANDLER %d\n", mode);
            mode_select = mode_tbl[mode].sim();
            // printf("BACK FROM  MODE HANDLER %d\n", mode);
        }
    }

    // exit 
    INFO("TERMINATING %s\n", argv[0]);
    sdl_terminate();
    return 0;
}

void display_handler_mode_select(void)
{
    int32_t i, event;
    SDL_Rect winpane;

    // clear window
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);

    // title
    SDL_INIT_PANE(winpane, 0, 0, sdl_win_width, sdl_win_height);
    sdl_render_text_font0(&winpane, 1, 10, "MODE SELECTION", SDL_EVENT_NONE);

    // menu list
    for (i = 0; i < MAX_MODE_TBL; i++) {
        sdl_render_text_font0(&winpane, 5+2*i, 10, mode_tbl[i].select_menu_name, SDL_EVENT_USER_START+i);
    }

    // present the display
    SDL_RenderPresent(sdl_renderer);

    // handle events
    while (true) {
        event = sdl_poll_event();
        switch (event) {
        case SDL_EVENT_NONE:
            usleep(10000);
            break;
        case SDL_EVENT_USER_START ... SDL_EVENT_USER_START+MAX_MODE_TBL-1:
            mode = event - SDL_EVENT_USER_START;
            mode_select = false;
            break;
        default:
            break;
        }

        if (event != SDL_EVENT_NONE) {
            break;
        }
    }
}
