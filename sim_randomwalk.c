#include "util.h"
#include "sim_randomwalk_help.h"

//
// defines
//

#define DEFAULT_DISPLAY_WIDTH 1000

#define DEFAULT_MAX_WALKER    1000
#define MIN_MAX_WALKER        1
#define MAX_MAX_WALKER        1000000 

#define DEFAULT_RUN_SPEED     9
#define MIN_RUN_SPEED         1
#define MAX_RUN_SPEED         9
#define RUN_SPEED_SLEEP_TIME (392 * ((1 << (MAX_RUN_SPEED-run_speed)) - 1))

#define MAX_CIRCLE_COLOR      (RED+1)

#define CIRCLE_RADIUS         2

#define SDL_EVENT_STOP               (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN                (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_SLOW               (SDL_EVENT_USER_START + 2)
#define SDL_EVENT_FAST               (SDL_EVENT_USER_START + 3)
#define SDL_EVENT_RESET              (SDL_EVENT_USER_START + 9)
#define SDL_EVENT_SELECT_PARAMS      (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_HELP               (SDL_EVENT_USER_START + 12)
#define SDL_EVENT_BACK               (SDL_EVENT_USER_START + 13)
#define SDL_EVENT_SIMPANE_ZOOM_IN    (SDL_EVENT_USER_START + 20)
#define SDL_EVENT_SIMPANE_ZOOM_OUT   (SDL_EVENT_USER_START + 21)

#define DISPLAY_NONE          0
#define DISPLAY_TERMINATE     1
#define DISPLAY_SIMULATION    2
#define DISPLAY_SELECT_PARAMS 3
#define DISPLAY_HELP          4

#define STATE_STOP               0
#define STATE_RUN                1
#define STATE_TERMINATE_THREADS  9

#define STATE_STR(s) \
    ((s) == STATE_STOP              ? "STOP"              : \
     (s) == STATE_RUN               ? "RUN"               : \
     (s) == STATE_TERMINATE_THREADS ? "TERMINATE_THREADS"   \
                                    : "????")

//
// typedefs
//

typedef struct {
    int32_t x;
    int32_t y;
} walker_t;

typedef struct {
    int64_t  steps;
    int32_t  max_walker;
    walker_t walker[0];
} sim_t;

//
// variables
//

static sim_t   * sim;
static int32_t   state; 
static int64_t   display_width;
static int32_t   run_speed;
static pthread_t thread_id;

// 
// prototypes
//

int32_t sim_randomwalk_init(int32_t max_walker);
void sim_randomwalk_terminate(bool do_print);
void sim_randomwalk_display(void);
int32_t sim_randomwalk_display_simulation(int32_t curr_display, int32_t last_display);
int32_t sim_randomwalk_display_select_params(int32_t curr_display, int32_t last_display);
int32_t sim_randomwalk_display_help(int32_t curr_display, int32_t last_display);
void * sim_randomwalk_thread(void * cx);

// -----------------  MAIN  -----------------------------------------------------

void sim_randomwalk(void) 
{
    display_width = DEFAULT_DISPLAY_WIDTH;
    run_speed     = DEFAULT_RUN_SPEED;

    sim_randomwalk_init(DEFAULT_MAX_WALKER);

    sim_randomwalk_display();

    sim_randomwalk_terminate(true);
}

// -----------------  INIT & TERMINATE  -----------------------------------------

int32_t sim_randomwalk_init(
    int32_t max_walker)
{
    #define SIM_SIZE(n) (sizeof(sim_t) + (n) * sizeof(walker_t))
    #define MB 0x100000

    // print info msg
    INFO("initialize start: max_walker=%d\n", max_walker);

    // ensure terminate
    sim_randomwalk_terminate(false);

    // allocate sim
    while (true) {
        sim = calloc(1, SIM_SIZE(max_walker));
        if (sim != NULL) {
            break;
        }
        ERROR("alloc sim failed, max_walker=%d size=%d MB, retrying with max_walker=%d\n",
              max_walker, (int32_t)(SIM_SIZE(max_walker)/MB), max_walker/2);
        max_walker /= 2;
    }

    // init 
    state = STATE_STOP;
    sim->max_walker = max_walker;
    pthread_create(&thread_id, NULL, sim_randomwalk_thread, NULL);

    // success
    INFO("initialize complete\n");
    return 0;
}

void sim_randomwalk_terminate(bool do_print)
{
    // print info msg
    if (do_print) {
        INFO("terminate start\n");
    }

    // terminate threads
    state = STATE_TERMINATE_THREADS;
    if (thread_id) {
        pthread_join(thread_id, NULL);
        thread_id = 0;
    }

    // free sim
    free(sim);
    sim = NULL;

    // done
    if (do_print) {
        INFO("terminate complete\n");
    }
}

// -----------------  DISPLAY  --------------------------------------------------

void sim_randomwalk_display(void)
{
    int32_t last_display = DISPLAY_NONE;
    int32_t curr_display = DISPLAY_SIMULATION;
    int32_t next_display = DISPLAY_SIMULATION;

    while (true) {
        switch (curr_display) {
        case DISPLAY_SIMULATION:
            next_display = sim_randomwalk_display_simulation(curr_display, last_display);
            break;
        case DISPLAY_SELECT_PARAMS:
            next_display = sim_randomwalk_display_select_params(curr_display, last_display);
            break;
        case DISPLAY_HELP:
            next_display = sim_randomwalk_display_help(curr_display, last_display);
            break;
        }

        if (sdl_quit || next_display == DISPLAY_TERMINATE) {
            break;
        }

        last_display = curr_display;
        curr_display = next_display;
    }
}

int32_t sim_randomwalk_display_simulation(int32_t curr_display, int32_t last_display)
{
    SDL_Rect      ctlpane, simpane, below_simpane;
    int32_t       simpane_width, color, i, disp_x, disp_y, next_display=-1;
    double        sum_distance, avg_distance, exp_distance;
    sdl_event_t * event;

    static SDL_Texture * circle_texture[MAX_CIRCLE_COLOR];
    static bool          circle_texture_initialized = false;

    //
    // first time called, init circle_texture
    //

    if (!circle_texture_initialized) {
        for (color = 0; color < MAX_CIRCLE_COLOR; color++) {
            circle_texture[color] = sdl_create_filled_circle_texture(CIRCLE_RADIUS, sdl_pixel_rgba[color]);
        }
        circle_texture_initialized = true;
    }

    //
    // loop until next_display has been set
    //

    while (next_display == -1) {
        //
        // short delay
        //

        usleep(5000);

        //
        // init simpane and ctlpane locations
        //

        if (sdl_win_width > sdl_win_height) {
            int32_t min_ctlpane_width = 18 * sdl_font[0].char_width;
            simpane_width = sdl_win_height;
            if (simpane_width + min_ctlpane_width > sdl_win_width) {
                simpane_width = sdl_win_width - min_ctlpane_width;
            }
            SDL_INIT_PANE(simpane,
                          0, 0,                                             // x, y
                          simpane_width, simpane_width);                    // w, h
            SDL_INIT_PANE(below_simpane,
                          0, simpane.h,                                    // x, y
                          simpane_width, sdl_win_height-simpane.h);       // w, h
            SDL_INIT_PANE(ctlpane,
                          simpane_width, 0,                                 // x, y
                          sdl_win_width-simpane_width, sdl_win_height);     // w, h
        } else {
            simpane_width = sdl_win_width;
            SDL_INIT_PANE(simpane, 0, 0, simpane_width, simpane_width);
            SDL_INIT_PANE(below_simpane, 0, simpane.h, simpane_width, sdl_win_height-simpane.h);
            SDL_INIT_PANE(ctlpane, 0, 0, 0, 0);
        }
        sdl_event_init();

        //
        // clear window
        //

        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        //
        // draw walkers in the simpane, and
        // compute the sum of all walkers distance from origin
        //

        sum_distance = 0;
        for (i = 0; i < sim->max_walker; i++) {
            walker_t * walker = &sim->walker[i];
            int32_t    x = walker->x;
            int32_t    y = walker->y;

            disp_x = simpane.x + (x + display_width/2) * simpane_width / display_width;
            disp_y = simpane.y + (y + display_width/2) * simpane_width / display_width;
            sdl_render_circle(disp_x, disp_y, circle_texture[i % MAX_CIRCLE_COLOR]);

            sum_distance += sqrt((int64_t)x*x + (int64_t)y*y);
        }

        //
        // clear area below simpane, if it exists
        //

        if (below_simpane.h > 0) {
            SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(sdl_renderer, &below_simpane);
        }

        //
        // clear ctlpane
        //

        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(sdl_renderer, &ctlpane);

        //
        // display status lines
        //

        // state & steps
        char str[100];
        sprintf(str, "%-6s %"PRId64, STATE_STR(state), sim->steps);
        sdl_render_text_font0(&ctlpane,  8, 0, str, SDL_EVENT_NONE);

        // average distance, expected distance, and ratio of these
        // note: http://math.stackexchange.com/questions/103142/expected-value-of-random-walk
        avg_distance = sum_distance / sim->max_walker;
        sprintf(str, "DIST   %d", (int32_t)avg_distance);
        sdl_render_text_font0(&ctlpane,  9, 0, str, SDL_EVENT_NONE);

        #define N_DIM 2.0
        exp_distance = sqrt(2*sim->steps/N_DIM) * tgamma((N_DIM+1)/2) / tgamma(N_DIM/2);
        sprintf(str, "EXP    %d", (int32_t)exp_distance);
        sdl_render_text_font0(&ctlpane, 10, 0, str, SDL_EVENT_NONE);

        if (exp_distance > 0) {
            sprintf(str, "RATIO  %1.3f", avg_distance / exp_distance);
        } else {
            sprintf(str, "RATIO ");
        }
        sdl_render_text_font0(&ctlpane, 11, 0, str, SDL_EVENT_NONE);

        // - params
        sdl_render_text_font0(&ctlpane, 13, 0, "PARAMS ...", SDL_EVENT_NONE);
        sprintf(str, "N_WLK  %d", sim->max_walker);    
        sdl_render_text_font0(&ctlpane, 14, 0, str, SDL_EVENT_NONE);

        // - window width
        sprintf(str, "WIDTH %"PRId64,  display_width);
        sdl_render_text_font0(&simpane,  0, 0, str, SDL_EVENT_NONE);

        //
        // display controls
        //

        char rs_str[20];
        sprintf(rs_str, "%d", run_speed);

        sdl_render_text_font0(&ctlpane,  0, 3,  "RANDOM WALK",  SDL_EVENT_NONE);
        sdl_render_text_font0(&ctlpane,  2, 0,  "RUN",       SDL_EVENT_RUN);
        sdl_render_text_font0(&ctlpane,  2, 7,  "STOP",      SDL_EVENT_STOP);
        sdl_render_text_font0(&ctlpane,  4, 0,  "SLOW",      SDL_EVENT_SLOW);
        sdl_render_text_font0(&ctlpane,  4, 7,  "FAST",      SDL_EVENT_FAST);
        sdl_render_text_font0(&ctlpane,  4, 14, rs_str,      SDL_EVENT_NONE);
        sdl_render_text_font0(&ctlpane,  6, 0,  "RESET",     SDL_EVENT_RESET);
        sdl_render_text_font0(&ctlpane,  6, 7,  "PARAMS",    SDL_EVENT_SELECT_PARAMS);
        sdl_render_text_font0(&ctlpane, -1, 0,  "HELP",      SDL_EVENT_HELP);
        sdl_render_text_font0(&ctlpane, -1,-5,  "BACK",      SDL_EVENT_BACK);

        sdl_render_text_font0(&simpane,  0, -5,  " + ", SDL_EVENT_SIMPANE_ZOOM_IN);
        sdl_render_text_font0(&simpane,  2, -5,  " - ", SDL_EVENT_SIMPANE_ZOOM_OUT);

        //
        // draw simpane border
        //

        sdl_render_rect(&simpane, 3, sdl_pixel_rgba[GREEN]);

        //
        // present the display
        //

        SDL_RenderPresent(sdl_renderer);

        //
        // handle events
        //

        event = sdl_poll_event();
        switch (event->event) {
        case SDL_EVENT_RUN:
            state = STATE_RUN;
            break;
        case SDL_EVENT_STOP:
            state = STATE_STOP;
            break;
        case SDL_EVENT_SLOW:
            if (run_speed > MIN_RUN_SPEED) {
                run_speed--;
            }
            break;
        case SDL_EVENT_FAST:
            if (run_speed < MAX_RUN_SPEED) {
                run_speed++;
            }
            break;
        case SDL_EVENT_RESET:
            state = STATE_STOP;
            sim_randomwalk_init(sim->max_walker);
            break;
        case SDL_EVENT_SELECT_PARAMS:
            state = STATE_STOP;
            next_display = DISPLAY_SELECT_PARAMS;
            break; 
        case SDL_EVENT_HELP: 
            state = STATE_STOP;
            next_display = DISPLAY_HELP;
            break;
        case SDL_EVENT_SIMPANE_ZOOM_OUT:
            if (display_width == 10000000) {
                break;
            }
            if ((display_width % 3) != 0) {
                display_width = display_width * 3;
            } else {
                display_width = display_width / 3 * 10;
            }
            break;
        case SDL_EVENT_SIMPANE_ZOOM_IN:
            if (display_width == 10) {
                break;
            }
            if ((display_width % 3) != 0) {
                display_width = display_width / 10 * 3;
            } else {
                display_width = display_width / 3;
            }
            break;
        case SDL_EVENT_BACK: 
        case SDL_EVENT_QUIT:
            state = STATE_STOP;
            next_display = DISPLAY_TERMINATE;
            break;
        }
    }

    //
    // return next_display
    //

    return next_display;
}

int32_t sim_randomwalk_display_select_params(int32_t curr_display, int32_t last_display)
{
    int32_t max_walker = sim->max_walker;
    char    cur_s1[100], ret_s1[100];

    // get new value strings for the params
    sprintf(cur_s1, "%d",    max_walker);
    sdl_display_get_string(1, "N_WLK", cur_s1, ret_s1);

    // scan returned string(s)
    if (sscanf(ret_s1, "%d", &max_walker) == 1) {
        if (max_walker < MIN_MAX_WALKER) max_walker = MIN_MAX_WALKER;
        if (max_walker > MAX_MAX_WALKER) max_walker = MAX_MAX_WALKER;
    }

    // re-init with new params
    sim_randomwalk_init(max_walker);

    // return next_display
    return sdl_quit ? DISPLAY_TERMINATE : DISPLAY_SIMULATION;
}

int32_t sim_randomwalk_display_help(int32_t curr_display, int32_t last_display)
{
    // display the help text
    sdl_display_text(sim_randomwalk_help);
    
    // return next_display
    return sdl_quit ? DISPLAY_TERMINATE : DISPLAY_SIMULATION;
}

// -----------------  THREAD  ---------------------------------------------------

void * sim_randomwalk_thread(void * cx)
{
    INFO("thread starting\n");
    
    while (true) {
        if (state == STATE_RUN) {
            int32_t i, direction;

            for (i = 0; i < sim->max_walker; i++) {
                direction = random() / (((unsigned)RAND_MAX+1)/4);
                switch (direction) {
                case 0:
                    sim->walker[i].x++;
                    break;
                case 1:
                    sim->walker[i].x--;
                    break;
                case 2:
                    sim->walker[i].y++;
                    break;
                case 3:
                    sim->walker[i].y--;
                    break;
                }
            }

            sim->steps++;

            usleep(RUN_SPEED_SLEEP_TIME);

        } else if (state == STATE_STOP) {
            usleep(50000);

        } else {  // state == STATE_TERMINATE_THREADS
            break;
        }
    }

    INFO("thread terminating\n");
    return NULL;
}
