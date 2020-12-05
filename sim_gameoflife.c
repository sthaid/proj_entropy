// XXX
// - how to pinch

#include "util.h"
#include "sim_gameoflife_help.h"

//
// defines
//

#define DEFAULT_WIDTH              100
#define DEFAULT_RUN_SPEED          4
#define DEFAULT_PARAM_INIT_PERCENT 12
#define DEFAULT_PARAM_RAND_SEED    2 

#define MAX_WIDTH 1000

#define MIN_RUN_SPEED        1
#define MAX_RUN_SPEED        9
#define RUN_SPEED_SLEEP_TIME (4096 * (1 << (MAX_RUN_SPEED-run_speed)))

#define SDL_EVENT_STOP               (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN                (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_SLOW               (SDL_EVENT_USER_START + 2)
#define SDL_EVENT_FAST               (SDL_EVENT_USER_START + 3)
#define SDL_EVENT_RESET              (SDL_EVENT_USER_START + 9)
#define SDL_EVENT_RESTART            (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_SELECT_PARAMS      (SDL_EVENT_USER_START + 11)
#define SDL_EVENT_HELP               (SDL_EVENT_USER_START + 12)
#define SDL_EVENT_BACK               (SDL_EVENT_USER_START + 13)
#define SDL_EVENT_SIMPANE_ZOOM_IN    (SDL_EVENT_USER_START + 20)
#define SDL_EVENT_SIMPANE_ZOOM_OUT   (SDL_EVENT_USER_START + 21)
#define SDL_EVENT_MOUSE_PAN          (SDL_EVENT_USER_START + 22)
#define SDL_EVENT_MOUSE_ZOOM         (SDL_EVENT_USER_START + 23)

#define DISPLAY_NONE          0
#define DISPLAY_TERMINATE     1
#define DISPLAY_SIMULATION    2
#define DISPLAY_SELECT_PARAMS 3
#define DISPLAY_HELP          4

//
// typedefs
//

typedef unsigned char (gen_t)[MAX_WIDTH][MAX_WIDTH];

//
// variables
//

static gen_t  *curr_gen;
static gen_t  *next_gen;

static bool    running;
static int32_t gen_count;
static int32_t width;
static int32_t ctr_row;
static int32_t ctr_col;
static int32_t run_speed;
static int32_t param_init_percent;
static int32_t param_rand_seed;

// 
// prototypes
//

static int32_t sim_init(bool reset);
static void sim_compute_next_gen(void);
static void set_curr_gen_cell_live_neightbors(int32_t r, int32_t c);
static void sim_set_next_gen_cell_live(int32_t r, int32_t c);
static void sim_set_next_gen_cell_dead(int32_t r, int32_t c);

static void display(void);
static int32_t display_simulation(int32_t curr_display, int32_t last_display);
static SDL_Rect *rc_to_rect(SDL_Rect *simpane, int32_t r, int32_t c);
static int32_t display_select_params(int32_t curr_display, int32_t last_display);
static int32_t display_help(int32_t curr_display, int32_t last_display);

// -----------------  MAIN  -----------------------------------------------------

void sim_gameoflife(void) 
{
    param_init_percent = DEFAULT_PARAM_INIT_PERCENT;
    param_rand_seed = DEFAULT_PARAM_RAND_SEED;
    curr_gen        = malloc(sizeof(gen_t));
    next_gen        = malloc(sizeof(gen_t));

    sim_init(true);

    display();

    free(curr_gen);
    free(next_gen);
}

// -----------------  SIMULATION  -----------------------------------------------

// xxx comment on cell format
// xxx comments on wikipedia

//
// sim defines
//

#define LIVE_MASK                   0x80
#define LIVE_WITH_2_LIVE_NEIGHBORS  (LIVE_MASK | 2)
#define LIVE_WITH_3_LIVE_NEIGHBORS  (LIVE_MASK | 3)
#define DEAD_WITH_3_LIVE_NEIGHBORS  (3)

#define INIT_PATTERN(array) \
    do { \
        int32_t i; \
        for (i = 0; i < sizeof(array)/sizeof(rc_t); i++) { \
            (*curr_gen)[(r)+(array)[i].r][(c)+(array)[i].c] = LIVE_MASK; \
        } \
        c += 15; if (c > 515) { r += 15; c = 485; } \
    } while (0)

//
// sim typedefs
//

typedef struct {
    int32_t r;
    int32_t c;
} rc_t;

//
// sim variables
//

// still lifes
rc_t block[]   = { {0,0}, {0,1}, {1,0}, {1,1} };
// oscillations
rc_t blinker[] = { {0,-1}, {0,0}, {0,1} };
rc_t toad[]    = { {0,-1}, {0,0}, {0,1}, {1,0}, {1,1}, {1,2} };
rc_t beacon[]  = { {-1,-1}, {-1,0}, {0,-1}, {0,0}, {1,1}, {1,2}, {2,1}, {2,2} };
rc_t penta_decathlon[] = { {5,0}, {4,-1}, {4,0}, {4,1}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, 
                           {-6,0}, {-5,-1}, {-5,0}, {-5,1}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2} };
// spaceships
rc_t glider[] = { {-1,-1}, {-1,1}, {0,0}, {0,1}, {1,0} };

rc_t lwss[]   = { {-1,0}, {-1,1}, {0,-2}, {0,-1}, {0,1}, {0,2}, {1,-2}, 
                  {1,-1}, {1,0}, {1,1}, {2,-1}, {2,0} };

// - - - - - - - - -  SIM - INIT - - - - - - - - - -

static int32_t sim_init(bool reset)
{
    int32_t r, c;

    if (reset) {
        width     = DEFAULT_WIDTH;
        run_speed = DEFAULT_RUN_SPEED;
        ctr_row   = MAX_WIDTH/2;
        ctr_col   = MAX_WIDTH/2;
    }

    // xxx
    running   = false;
    gen_count = 0;

    // xxx
    bzero(curr_gen, sizeof(gen_t));
    bzero(next_gen, sizeof(gen_t));
    if (param_init_percent == 0) {
        r = 485, c = 485;
        INIT_PATTERN(block);
        INIT_PATTERN(blinker);
        INIT_PATTERN(toad);
        INIT_PATTERN(beacon);
        INIT_PATTERN(penta_decathlon);
        INIT_PATTERN(glider);
        INIT_PATTERN(lwss);
    } else {
        // xxx later
        int32_t tmp = param_init_percent * 1024 / 100;
        srandom(param_rand_seed);
        for (r = 1; r < MAX_WIDTH-1; r++) {
            for (c = 1; c < MAX_WIDTH-1; c++) {
                if ((random() & 1023) < tmp) {
                    (*curr_gen)[r][c] = LIVE_MASK;
                }
            }
        }
    }

    // xxx
    for (r = 1; r < MAX_WIDTH-1; r++) {
        for (c = 1; c < MAX_WIDTH-1; c++) {
            set_curr_gen_cell_live_neightbors(r, c);
        }
    }

    // success
    return 0;
}

// - - - - - - - - -  SIM - COMPUTE NEXT GEN - - - - - - - - - - -

static void sim_compute_next_gen(void)
{
    int32_t r, c;
    unsigned char cell;

    // https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
    // 
    // 1. Any live cell with two or three live neighbours survives.
    // 2. Any dead cell with three live neighbours becomes a live cell.
    // 3. All other live cells die in the next generation. Similarly, all other dead cells stay dead.

    memcpy(next_gen, curr_gen, sizeof(gen_t));

    for (r = 1; r < MAX_WIDTH-1; r++) {
        for (c = 1; c < MAX_WIDTH-1; c++) {
            cell = (*curr_gen)[r][c];
            if (cell == LIVE_WITH_2_LIVE_NEIGHBORS || cell == LIVE_WITH_3_LIVE_NEIGHBORS) {
                // survives in next gen
            } else if (cell == DEAD_WITH_3_LIVE_NEIGHBORS) {
                // set to live in next_gen
                sim_set_next_gen_cell_live(r, c);
            } else if (cell & LIVE_MASK) {
                // set to dead in next_gen
                sim_set_next_gen_cell_dead(r, c);
            }
        }
    }

    SWAP(curr_gen, next_gen);

    // xxx sanity check next gen neighbors count
}

// - - - - - - - - -  SIM - SUPPORT  - - - - - - - -

static void set_curr_gen_cell_live_neightbors(int32_t r, int32_t c)
{
    int32_t live_neighbors = 0;

    if ((*curr_gen)[r-1][c-1] & LIVE_MASK) live_neighbors++;
    if ((*curr_gen)[r-1][c  ] & LIVE_MASK) live_neighbors++;
    if ((*curr_gen)[r-1][c+1] & LIVE_MASK) live_neighbors++;
    if ((*curr_gen)[r  ][c-1] & LIVE_MASK) live_neighbors++;
    if ((*curr_gen)[r  ][c+1] & LIVE_MASK) live_neighbors++;
    if ((*curr_gen)[r+1][c-1] & LIVE_MASK) live_neighbors++;
    if ((*curr_gen)[r+1][c  ] & LIVE_MASK) live_neighbors++;
    if ((*curr_gen)[r+1][c+1] & LIVE_MASK) live_neighbors++;

    (*curr_gen)[r][c] = ((*curr_gen)[r][c] & LIVE_MASK) | live_neighbors;
}

static void sim_set_next_gen_cell_live(int32_t r, int32_t c)
{
    // fatal error if the next_gen cell is already live
    if ((*next_gen)[r][c] & LIVE_MASK) {
        FATAL("cell already live %d,%d\n", r,c);
    }

    // set the cell live
    (*next_gen)[r][c] |= LIVE_MASK;

    // for all neighbors, increment their neightbor count
    (*next_gen)[r-1][c-1]++;
    (*next_gen)[r-1][c  ]++;
    (*next_gen)[r-1][c+1]++;
    (*next_gen)[r  ][c-1]++;
    (*next_gen)[r  ][c+1]++;
    (*next_gen)[r+1][c-1]++;
    (*next_gen)[r+1][c  ]++;
    (*next_gen)[r+1][c+1]++;
}

static void sim_set_next_gen_cell_dead(int32_t r, int32_t c)
{
    // fatal error if the next_gen cell is already dead
    if (((*next_gen)[r][c] & LIVE_MASK) == 0) {
        FATAL("cell already dead %d,%d\n", r,c);
    }

    // set the cell dead
    (*next_gen)[r][c] &= ~LIVE_MASK;

    // for all neighbors, decrement their neightbor count
    (*next_gen)[r-1][c-1]--;
    (*next_gen)[r-1][c  ]--;
    (*next_gen)[r-1][c+1]--;
    (*next_gen)[r  ][c-1]--;
    (*next_gen)[r  ][c+1]--;
    (*next_gen)[r+1][c-1]--;
    (*next_gen)[r+1][c  ]--;
    (*next_gen)[r+1][c+1]--;
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
    SDL_Rect      ctlpane, simpane;
    int32_t       next_display=-1;
    sdl_event_t * event;
    int32_t       r, c, r_start, c_start;
    char          str[100];

    uint64_t time_now, time_of_last_compute=0;

    // loop until next_display has been set
    while (next_display == -1) {
        // short delay, to not use 100% cpu
        usleep(1000);

        // compute the next generation
        time_now = microsec_timer();
        if (running && time_now - time_of_last_compute > RUN_SPEED_SLEEP_TIME) {
            //uint64_t start_us = microsec_timer();
            sim_compute_next_gen();
            //if (gen_count < 100) INFO("duration = %ld\n", microsec_timer() - start_us);
            gen_count++;
            time_of_last_compute = time_now;
        }

        // init simpane and ctlpane locations
        SDL_INIT_PANE(simpane,
                      0, 0,                                          // x, y
                      sdl_win_height, sdl_win_height);               // w, h
        SDL_INIT_PANE(ctlpane,
                      sdl_win_height, 0,                             // x, y
                      sdl_win_width-sdl_win_height, sdl_win_height); // w, h

        // xxx
        sdl_event_init();

        // clear window
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // draw the current generation
        SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); //xxx, should have a routine
        r_start = ctr_row - width/2;
        c_start = ctr_col - width/2;
        for (r = r_start; r <= r_start+width-1; r++) {
            if (r < 0) continue;
            if (r >= MAX_WIDTH) break;
            for (c = c_start; c <= c_start+width-1; c++) {
                if (c < 0) continue;
                if (c >= MAX_WIDTH) break;
                if ((*curr_gen)[r][c] & LIVE_MASK) {
                    SDL_Rect *rect = rc_to_rect(&simpane, r-r_start, c-c_start);
                    SDL_RenderFillRect(sdl_renderer, rect);
                }
            }
        }

        // draw simpane border
        sdl_render_rect(&simpane, 3, sdl_pixel_rgba[GREEN]);
        
        // display ctlpane ...
        // - row 0:  GAME OF LIFE
        sdl_render_text_font0(&ctlpane,  0, 3,  "GAME OF LIFE",  SDL_EVENT_NONE);

        // - row 2:  RUN|CONT|PAUSE   RESET|RESTART   PARAMS
        if (running == false && gen_count == 0) {
            sdl_render_text_font0(&ctlpane,  2, 0,  "RUN",       SDL_EVENT_RUN);
        } else if (running == false && gen_count != 0) {
            sdl_render_text_font0(&ctlpane,  2, 0,  "CONT",      SDL_EVENT_RUN);
        } else {
            sdl_render_text_font0(&ctlpane,  2, 0,  "PAUSE",       SDL_EVENT_STOP);
        }
        sdl_render_text_font0(&ctlpane,  2, 7,  "RESTART",     SDL_EVENT_RESTART);
        sdl_render_text_font0(&ctlpane,  2, 16,  "RESET",     SDL_EVENT_RESET);

        // - row 4:  SLOW   FAST   n
        sdl_render_text_font0(&ctlpane,  4, 0,  "SLOW",      SDL_EVENT_SLOW);
        sdl_render_text_font0(&ctlpane,  4, 7,  "FAST",      SDL_EVENT_FAST);
        sprintf(str, "%d", run_speed);
        sdl_render_text_font0(&ctlpane,  4, 16, str,      SDL_EVENT_NONE);

        // - row 6:  ZOOM+  ZOOM-
        sdl_render_text_font0(&ctlpane,  6, 0,  "ZOOM+", SDL_EVENT_SIMPANE_ZOOM_IN);
        sdl_render_text_font0(&ctlpane,  6, 7,  "ZOOM-", SDL_EVENT_SIMPANE_ZOOM_OUT);
        sprintf(str, "%d", width);
        sdl_render_text_font0(&ctlpane,  6, 16, str,      SDL_EVENT_NONE);

        // - row 8:  RUNNING|PAUSED|RESET   nnnn
        if (running) {
            sprintf(str, "RUNNING %d", gen_count);
        } else if (gen_count > 0) {
            sprintf(str, "PAUSED %d", gen_count);
        } else {
            sprintf(str, "RESET");
        }
        sdl_render_text_font0(&ctlpane,  8, 0, str,      SDL_EVENT_NONE);

        // - row 10: PARAMS ...
        // - row 11: - INIT    n
        sdl_render_text_font0(&ctlpane, 10, 0, "PARAMS ...", SDL_EVENT_SELECT_PARAMS);
        sprintf(str, "INIT_PERCENT %d", param_init_percent);
        sdl_render_text_font0(&ctlpane, 11, 0, str, SDL_EVENT_NONE);
        sprintf(str, "RAND_SEED    %d", param_rand_seed);
        sdl_render_text_font0(&ctlpane, 12, 0, str, SDL_EVENT_NONE);

        // - row 14: DEBUG
        //sprintf(str, "%d %d", ctr_row, ctr_col);
        //sdl_render_text_font0(&ctlpane, 14, 0, str, SDL_EVENT_NONE);

        // - row N:  HELP   BACK
        sdl_render_text_font0(&ctlpane, -1, 0,  "HELP",      SDL_EVENT_HELP);
        sdl_render_text_font0(&ctlpane, -1,-5,  "BACK",      SDL_EVENT_BACK);

        // xxx
        sdl_event_register(SDL_EVENT_MOUSE_PAN, SDL_EVENT_TYPE_MOUSE_MOTION, &simpane);
        sdl_event_register(SDL_EVENT_MOUSE_ZOOM, SDL_EVENT_TYPE_MOUSE_WHEEL, &simpane);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // handle events
        event = sdl_poll_event();
        switch (event->event) {
        case SDL_EVENT_RUN:
            running = true;
            break;
        case SDL_EVENT_STOP:
            running = false;
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
            sim_init(true);
            break;
        case SDL_EVENT_RESTART:
            sim_init(false);
            running = true;
            break;
        case SDL_EVENT_SELECT_PARAMS:
            next_display = DISPLAY_SELECT_PARAMS;
            break; 
        case SDL_EVENT_HELP: 
            next_display = DISPLAY_HELP;
            break;
        case SDL_EVENT_SIMPANE_ZOOM_OUT:
            width += 10;
            if (width > 500) width = 500;
            break;
        case SDL_EVENT_SIMPANE_ZOOM_IN:
            width -= 10;
            if (width < 10) width = 10;
            break;
        case SDL_EVENT_MOUSE_ZOOM:
            if (event->mouse_wheel.delta_y < 0) {
                width += 10;
                if (width > 500) width = 500;
            } else if (event->mouse_wheel.delta_y > 0) {
                width -= 10;
                if (width < 10) width = 10;
            }
            break;
        case SDL_EVENT_MOUSE_PAN: {
            static double tmp_col, tmp_row;
            double square_width = simpane.w / width;

            tmp_col -= (event->mouse_motion.delta_x);
            tmp_row -= (event->mouse_motion.delta_y);

            while (tmp_col > square_width) {
                tmp_col -= square_width;
                ctr_col++;
            } 
            while (tmp_col < -square_width) {
                tmp_col += square_width;
                ctr_col--;
            }

            while (tmp_row > square_width) {
                tmp_row -= square_width;
                ctr_row++;
            } 
            while (tmp_row < -square_width) {
                tmp_row += square_width;
                ctr_row--;
            }
            break; }
        case SDL_EVENT_BACK: 
        case SDL_EVENT_QUIT:
            running = false;
            next_display = DISPLAY_TERMINATE;
            break;
        }
    }

    // return next_display
    return next_display;
}

// xxx name for r,c
static SDL_Rect *rc_to_rect(SDL_Rect *simpane, int32_t r, int32_t c)
{
    static int32_t      sq_beg[MAX_WIDTH+1];
    static int32_t      width_save;
    static SDL_Rect simpane_save;
    static SDL_Rect rect;

    int32_t i;
    double tmp;

    if (width != width_save || simpane->w != simpane_save.w) {
        tmp = (double)(simpane->w - 5) / width;
        for (i = 0; i < width; i++) {
            sq_beg[i] = rint(3 + i * tmp);
        }
        sq_beg[width] = simpane->w - 2;

        width_save = width;
        simpane_save = *simpane;
    }

    rect.x = sq_beg[c] + simpane->x;
    rect.y = sq_beg[r] + simpane->y;
    rect.w = sq_beg[c+1] - sq_beg[c] - 1;
    rect.h = sq_beg[r+1] - sq_beg[r] - 1;

    return &rect;
}

// - - - - - - - - -  DISPLAY : SELECT_PARAMS  - - - - - - - - - - - - - - - -

static int32_t display_select_params(int32_t curr_display, int32_t last_display)
{
    char cur_s1[100], ret_s1[100];
    char cur_s2[100], ret_s2[100];

    // get new value strings for the params
    sprintf(cur_s1, "%d", param_init_percent);
    sprintf(cur_s2, "%d", param_rand_seed);
    sdl_display_get_string(2, "INIT_PERCENT", cur_s1, ret_s1, "RAND_SEED", cur_s2, ret_s2);

    // scan returned string(s)
    sscanf(ret_s1, "%d", &param_init_percent);
    sscanf(ret_s2, "%d", &param_rand_seed);

    // constrain values to their allowed range
    if (param_init_percent < 0) param_init_percent = 0;
    if (param_init_percent > 100) param_init_percent = 100;

    // re-init with new params
    sim_init(true);

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
