#include "util.h"
#include "sim_gameoflife_help.h"

//
// defines
//

#define DEFAULT_WIDTH         51
#define DEFAULT_RUN_SPEED     5
#define DEFAULT_PARAM         0  // xxx

#define MAX_WIDTH             1000

#define MIN_RUN_SPEED         1
#define MAX_RUN_SPEED         9
#define RUN_SPEED_SLEEP_TIME (392 * ((1 << (MAX_RUN_SPEED-run_speed)) - 1))

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

#define STATE_STOP  0
#define STATE_RUN   1

#define STATE_STR(s) \
    ((s) == STATE_STOP ? "STOP" : \
     (s) == STATE_RUN  ? "RUN"  : \
                         "????")

//
// typedefs
//

typedef unsigned char (gen_t)[MAX_WIDTH][MAX_WIDTH];

//
// variables
//

static gen_t  *curr_gen;
static gen_t  *next_gen;
static int32_t gen_count;

static int32_t state; 
static int32_t width;
static int32_t run_speed;

static int32_t param;

// 
// prototypes
//

static int32_t sim_init(void);
static void display(void);
static int32_t display_simulation(int32_t curr_display, int32_t last_display);
static SDL_Rect *rc_to_rect(SDL_Rect *simpane, int r, int c, int start);
static int32_t display_select_params(int32_t curr_display, int32_t last_display);
static int32_t display_help(int32_t curr_display, int32_t last_display);

// -----------------  MAIN  -----------------------------------------------------

void sim_gameoflife(void) 
{
    width     = DEFAULT_WIDTH;
    run_speed = DEFAULT_RUN_SPEED;
    param     = DEFAULT_PARAM;
    curr_gen  = malloc(sizeof(gen_t));
    next_gen  = malloc(sizeof(gen_t));

    sim_init();

    display();

    free(curr_gen);
    free(next_gen);
}

// -----------------  SIMULATION  -----------------------------------------------

// XXX 
static int32_t sim_init(void)
{
    // init 
    state = STATE_STOP;

    gen_count = 0;
    bzero(curr_gen, sizeof(gen_t));
    bzero(next_gen, sizeof(gen_t));

    // init curr_gen xxx
#if 1
    int r,c;
    for (r = 0; r < MAX_WIDTH; r++) {
        for (c = 0; c < MAX_WIDTH; c++) {
            if ((r+c) & 1) {
                (*curr_gen)[r][c] = 0x80;
            }
        }
    }
#endif
    //(*curr_gen)[500][500] = 0x80;

    // success
    INFO("initialize complete\n");
    return 0;
}

static inline bool sim_curr_gen_live_cell(int r, int c)
{
    return (*curr_gen)[r][c] & 0x80;  // xxx define
}

// -----------------  DISPLAY  --------------------------------------------------

static void display(void)
{
    int32_t last_display = DISPLAY_NONE;
    int32_t curr_display = DISPLAY_SIMULATION;
    int32_t next_display = DISPLAY_SIMULATION;

    while (true) {
        switch (curr_display) {
        case DISPLAY_SIMULATION:
            next_display = display_simulation(curr_display, last_display);
            break;
        case DISPLAY_SELECT_PARAMS:
            next_display = display_select_params(curr_display, last_display);
            break;
        case DISPLAY_HELP:
            next_display = display_help(curr_display, last_display);
            break;
        }

        if (sdl_quit || next_display == DISPLAY_TERMINATE) {
            break;
        }

        last_display = curr_display;
        curr_display = next_display;
    }
}

// - - - - - - - - -  DISPLAY : SIMULATION   - - - - - - - - - - - - - - - - -

static int32_t display_simulation(int32_t curr_display, int32_t last_display)
{
    SDL_Rect      ctlpane, simpane, below_simpane;
    int32_t       next_display=-1;
    sdl_event_t * event;
    int32_t       r, c, start, end;

    // loop until next_display has been set
    while (next_display == -1) {
        // short delay
        usleep(1000);

        // init simpane and ctlpane locations
        // xxx test rotation
        if (sdl_win_width > sdl_win_height) {
            int32_t min_ctlpane_width = 18 * sdl_font[0].char_width;
            int32_t simpane_width = sdl_win_height;
            if (simpane_width + min_ctlpane_width > sdl_win_width) {
                simpane_width = sdl_win_width - min_ctlpane_width;
            }
            SDL_INIT_PANE(simpane,
                          0, 0,                                          // x, y
                          simpane_width, simpane_width);                 // w, h
            SDL_INIT_PANE(below_simpane,
                          0, simpane.h,                                  // x, y
                          simpane_width, sdl_win_height-simpane.h);      // w, h
            SDL_INIT_PANE(ctlpane,
                          simpane_width, 0,                              // x, y
                          sdl_win_width-simpane_width, sdl_win_height);  // w, h
        } else {
            int32_t simpane_width = sdl_win_width;
            SDL_INIT_PANE(simpane, 0, 0, simpane_width, simpane_width);
            SDL_INIT_PANE(below_simpane, 0, simpane.h, simpane_width, sdl_win_height-simpane.h);
            SDL_INIT_PANE(ctlpane, 0, 0, 0, 0);
        }
        sdl_event_init();

        // clear window
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // draw the current generation
        // xxx assert width is odd
        SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); //xxx, should have a routine
        start = MAX_WIDTH/2 - width/2;
        end   = start + width - 1;
        for (r = start; r <= end; r++) {
            for (c = start; c <= end; c++) {
                if (sim_curr_gen_live_cell(r,c)) {
                    SDL_Rect *rect = rc_to_rect(&simpane, r, c, start);
                    SDL_RenderFillRect(sdl_renderer, rect);
                }
            }
        }

        // compute the next generation
        // XXX if running AND time has elpased
        //    compute_next_gen();
        
        // clear area below simpane, if it exists
        if (below_simpane.h > 0) {
            SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(sdl_renderer, &below_simpane);
        }

        // clear ctlpane
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(sdl_renderer, &ctlpane);

        // display status lines ...
        //
        // state & steps
        char str[100];
        sprintf(str, "%-6s %d", STATE_STR(state), gen_count);
        sdl_render_text_font0(&ctlpane,  8, 0, str, SDL_EVENT_NONE);

        // params
        sdl_render_text_font0(&ctlpane, 13, 0, "PARAMS ...", SDL_EVENT_NONE);
        sprintf(str, "PARAM  %d", param);     // xxx name tbd
        sdl_render_text_font0(&ctlpane, 14, 0, str, SDL_EVENT_NONE);

        // window width  xxx don't print this, later
        sprintf(str, "WIDTH %d",  width);
        sdl_render_text_font0(&simpane,  0, 0, str, SDL_EVENT_NONE);

        // display controls
        char rs_str[20];
        sprintf(rs_str, "%d", run_speed);

        sdl_render_text_font0(&ctlpane,  0, 3,  "GAME OF LIFE",  SDL_EVENT_NONE);
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

        // draw simpane border
        sdl_render_rect(&simpane, 3, sdl_pixel_rgba[GREEN]);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // handle events
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
            sim_init();
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
            width += 20;  // xxx later
            break;
        case SDL_EVENT_SIMPANE_ZOOM_IN:
            width -= 20;  //xxx
            break;
        case SDL_EVENT_BACK: 
        case SDL_EVENT_QUIT:
            state = STATE_STOP;
            next_display = DISPLAY_TERMINATE;
            break;
        }
    }

    // return next_display
    return next_display;
}

static SDL_Rect *rc_to_rect(SDL_Rect *simpane, int r, int c, int start)
{
    static int sq_beg[MAX_WIDTH+1];
    static int width_save;
    static SDL_Rect rect;

    int i;
    double tmp;

    if (width != width_save) {
        tmp = (double)(simpane->w - 6) / width;
        for (i = 0; i < width; i++) {
            sq_beg[i] = rint(3 + i * tmp);
        }
        sq_beg[width] = simpane->w - 3;

        width_save = width;
    }

    r -= start;
    c -= start;

    rect.x = sq_beg[c];
    rect.y = sq_beg[r];
    rect.w = sq_beg[c+1] - sq_beg[c];
    rect.h = sq_beg[r+1] - sq_beg[r];

    return &rect;
}

// - - - - - - - - -  DISPLAY : SELECT_PARAMS  - - - - - - - - - - - - - - - -

static int32_t display_select_params(int32_t curr_display, int32_t last_display)
{
    char cur_s1[100], ret_s1[100];

    // get new value strings for the params
    sprintf(cur_s1, "%d", param);
    sdl_display_get_string(1, "PARAM", cur_s1, ret_s1);

    // scan returned string(s)
    if (sscanf(ret_s1, "%d", &param) == 1) {
        // xxx tbd
    }

    // re-init with new params
    sim_init();

    // return next_display
    return sdl_quit ? DISPLAY_TERMINATE : DISPLAY_SIMULATION;
}

// - - - - - - - - -  DISPLAY : HELP  - - - - - - - - - - - - - - - - - - - -

static int32_t display_help(int32_t curr_display, int32_t last_display)
{
    // display the help text
    sdl_display_text(sim_gameoflife_help);
    
    // return next_display
    return sdl_quit ? DISPLAY_TERMINATE : DISPLAY_SIMULATION;
}
