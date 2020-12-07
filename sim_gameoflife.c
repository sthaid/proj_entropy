#include "util.h"
#include "sim_gameoflife_help.h"

//
// defines
//

//#define UNIT_TEST

#define DEFAULT_WIDTH              100
#define DEFAULT_RUN_SPEED          5
#define DEFAULT_PARAM_INIT         0 

#define MAX_WIDTH 1000

#define MIN_RUN_SPEED        1
#define MAX_RUN_SPEED        9
#define RUN_SPEED_SLEEP_TIME (8192 * (1 << (MAX_RUN_SPEED-run_speed)))

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
static int32_t param_init;

// 
// prototypes
//

static void sim_init(void);
static void sim_compute_next_gen(void);
static void set_curr_gen_cell_live_neighbor_count(int32_t r, int32_t c);
static void sim_set_next_gen_cell_live(int32_t r, int32_t c);
static void sim_set_next_gen_cell_dead(int32_t r, int32_t c);
#ifdef UNIT_TEST
static int32_t get_next_gen_cell_live_neighbor_count(int32_t r, int32_t c);
#endif

static void display(void);
static int32_t display_simulation(int32_t curr_display, int32_t last_display);
static SDL_Rect *rc_to_rect(SDL_Rect *simpane, int32_t r, int32_t c);
static void display_sim_reset(void);
static int32_t display_select_params(int32_t curr_display, int32_t last_display);
static int32_t display_help(int32_t curr_display, int32_t last_display);

// -----------------  MAIN  -----------------------------------------------------

void sim_gameoflife(void) 
{
    curr_gen   = malloc(sizeof(gen_t));
    next_gen   = malloc(sizeof(gen_t));
    param_init = DEFAULT_PARAM_INIT;

    display_sim_reset();
    sim_init();

    display();

    free(curr_gen);
    free(next_gen);
}

// -----------------  SIMULATION  -----------------------------------------------

//
// notes:
// 1) Reference Wikipedia article:
//    https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
// 2) Cell format:
//    bit 7:     when set the cell is Live, else Dead
//    bits 0..6: live neighbor count
// 3) Cells at the perimeter are not used, this is 
//    accomplished with loop statements such as:
//      for (r = 1; r < MAX_WIDTH-1; r++)
//    This is done so that routines, such as, 
//    sim_set_next_gen_cell_live() do not need to check for
//    their r,c args being on the perimeter.
//

//
// sim defines
//

#define LIVE_MASK                   0x80
#define LIVE_WITH_2_LIVE_NEIGHBORS  (LIVE_MASK | 2)
#define LIVE_WITH_3_LIVE_NEIGHBORS  (LIVE_MASK | 3)
#define DEAD_WITH_3_LIVE_NEIGHBORS  (3)

#define INIT_PATTERN(_r,_c,_array) \
    do { \
        int32_t i, r, c; \
        r = (_r) + MAX_WIDTH/2; \
        c = (_c) + MAX_WIDTH/2; \
        for (i = 0; i < sizeof(_array)/sizeof(rc_t); i++) { \
            (*curr_gen)[(r)+(_array)[i].r][(c)+(_array)[i].c] = LIVE_MASK; \
        } \
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
rc_t pulsar[] = {
            {-6,-4}, {-6,-3}, {-6,-2}, {-6,2}, {-6,3}, {-6,4},
            {-4,-6}, {-4,-1}, {-4,1}, {-4,6},
            {-3,-6}, {-3,-1}, {-3,1}, {-3,6},
            {-2,-6}, {-2,-1}, {-2,1}, {-2,6},
            {-1,-4}, {-1,-3}, {-1,-2}, {-1,2}, {-1,3}, {-1,4},
            {1,-4}, {1,-3}, {1,-2}, {1,2}, {1,3}, {1,4},
            {2,-6}, {2,-1}, {2,1}, {2,6},
            {3,-6}, {3,-1}, {3,1}, {3,6},
            {4,-6}, {4,-1}, {4,1}, {4,6},
            {6,-4}, {6,-3}, {6,-2}, {6,2}, {6,3}, {6,4} };

// spaceships
rc_t glider[] = { {-1,-1}, {-1,1}, {0,0}, {0,1}, {1,0} };
rc_t lwss[]   = { {-1,0}, {-1,1}, {0,-2}, {0,-1}, {0,1}, {0,2}, {1,-2}, 
                  {1,-1}, {1,0}, {1,1}, {2,-1}, {2,0} };
rc_t mwss[]   = { {-2,0}, {-1,-2}, {-1,2}, {0,3}, {1,-2}, {1,3}, 
                  {2,-1}, {2,0}, {2,1}, {2,2}, {2,3} };
rc_t hwss[]   = { {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3},
                  {-1,-3}, {-1,3}, {0,3}, {1,-3}, {1,2}, {2,-1}, {2,0} };

//  Methuselahs  (evolve for long periods before stabilizing)
rc_t r_pentomino[] = { {-1,0}, {-1,1}, {0,-1}, {0,0}, {1,0} };
rc_t diehard[] = { {-1,2}, {0,-4}, {0,-3}, {1,-3}, {1,1}, {1,2}, {1,3} };
rc_t acorn[] = { {-1,-2}, {0,0}, {1,-3}, {1,-2}, {1,1}, {1,2}, {1,3} };

// glider guns
rc_t gosper_glider_gun[] = {
        {-5,7},
        {-4,5}, {-4,7},
        {-3,-5}, {-3,-4}, {-3,3}, {-3,4}, {-3,17}, {-3,18},
        {-2,-6}, {-2,-2}, {-2,3}, {-2,4}, {-2,17}, {-2,18},
        {-1,-17}, {-1,-16}, {-1,-7}, {-1,-1}, {-1,3}, {-1,4},
        {0,-17}, {0,-16}, {0,-7}, {0,-3}, {0,-1}, {0,0}, {0,5}, {0,7},
        {1,-7}, {1,-1}, {1,7},
        {2,-6}, {2,-2},
        {3,-5}, {3,-4} };
rc_t simkin_glider_gun[] = {
        {-10,-15}, {-10,-14}, {-10,-8}, {-10,-7},
        {-9,-15}, {-9,-14}, {-9,-8}, {-9,-7},
        {-7,-11}, {-7,-10},
        {-6,-11}, {-6,-10},
        {-1,7}, {-1,8}, {-1,10}, {-1,11},
        {0,6}, {0,12},
        {1,6}, {1,13}, {1,16}, {1,17},
        {2,6}, {2,7}, {2,8}, {2,12}, {2,16}, {2,17},
        {3,11},
        {7,5}, {7,6},
        {8,5},
        {9,6}, {9,7}, {9,8},
        {10,8} };

// infinite growth
rc_t inf_growth1[] = {
        {-3,2},
        {-2,0}, {-2,2}, {-2,3},
        {-1,0 }, {-1,2},
        {0,0},
        {1,-2},
        {2,-4}, {2,-2} };
rc_t inf_growth2[] = {
        {-2,-2}, {-2,-1}, {-2,0}, {-2,2},
        {-1,-2},
        {0,1}, {0,2},
        {1,-1}, {1,0}, {1,2},
        {2,-2}, {2,0}, {2,2} };
rc_t inf_growth3[] = {
        {0,-17}, {0,-16}, {0,-15}, {0,-14}, {0,-13}, {0,-12}, {0,-11}, {0,-10},
        {0,-8}, {0,-7}, {0,-6}, {0,-5}, {0,-4},
        {0,0}, {0,1}, {0,2},
        {0,9}, {0,10}, {0,11}, {0,12}, {0,13}, {0,14}, {0,15},
        {0,17}, {0,18}, {0,19}, {0,20}, {0,21} };

// - - - - - - - - -  SIM - INIT - - - - - - - - - -

static void sim_init(void)
{
    // set to not running, and clear generation count
    running   = false;
    gen_count = 0;

    // init current generation (curr_gen) array;
    // - when param_init is 0 then cell test patterns from the Wikipedia
    //   article are used
    // - otherwise, randomly set param_init fraction of the cells to live
    bzero(curr_gen, sizeof(gen_t));
    bzero(next_gen, sizeof(gen_t));

// XXX title, and select without using params
    switch (param_init) {
    case 0:
        INIT_PATTERN(-15, -15, block);
        INIT_PATTERN(-15,   0, blinker);
        INIT_PATTERN(-15,  15, lwss);
        INIT_PATTERN(  0, -15, toad);
        INIT_PATTERN(  0,   0, beacon);
        INIT_PATTERN(  0,  15, mwss);
        INIT_PATTERN( 15, -15, penta_decathlon);
        INIT_PATTERN( 15,   0, pulsar);
        INIT_PATTERN( 15,  15, hwss);
        break;
    case 1:
        INIT_PATTERN(0, 0, r_pentomino);
        break;
    case 2:
        INIT_PATTERN(0, 0, diehard);
        break;
    case 3:
        INIT_PATTERN(0, 0, acorn);
        break;
    case 4:
        INIT_PATTERN(0, 0, gosper_glider_gun);
        break;
    case 5:
        INIT_PATTERN(0, 0, simkin_glider_gun);
        break;
    case 6:
        INIT_PATTERN(0, 0, inf_growth1);
        break;
    case 7:
        INIT_PATTERN(0, 0, inf_growth2);
        break;
    case 8:
        INIT_PATTERN(0, 0, inf_growth3);
        break;
    default:
        break;
    }
#if 0
    // xxx tbd
    else {
        int32_t tmp = param_init * 1024 / 100;
        srandom(param_rand_seed);
        for (r = 1; r < MAX_WIDTH-1; r++) {
            for (c = 1; c < MAX_WIDTH-1; c++) {
                if ((random() & 1023) < tmp) {
                    (*curr_gen)[r][c] = LIVE_MASK;
                }
            }
        }
    }
#endif

    // set each curr_gen cell's live neighbor count;
    //
    // Note that in this case the entire curr_gen array
    // needs to be initialized with the live neighbor count.
    // Code in set_curr_gen_cell_live_neighbor_count checks for
    // the r,c being on the perimeter.
    int32_t r,c;
    for (r = 0; r < MAX_WIDTH; r++) {
        for (c = 0; c < MAX_WIDTH; c++) {
            set_curr_gen_cell_live_neighbor_count(r, c);
        }
    }
}

// - - - - - - - - -  SIM - COMPUTE NEXT GEN - - - - - - - - - - -

static void sim_compute_next_gen(void)
{
    int32_t r, c;
    unsigned char cell;

    // https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life
    // 1. Any live cell with two or three live neighbours survives.
    // 2. Any dead cell with three live neighbours becomes a live cell.
    // 3. All other live cells die in the next generation. Similarly, all other dead cells stay dead.

    // start with next_gen being a copy of curr_gen
    memcpy(next_gen, curr_gen, sizeof(gen_t));

    // update fields in next_gen per the above description 
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

#ifdef UNIT_TEST
    // sanity check next gen neighbors count
    for (r = 1; r < MAX_WIDTH-1; r++) {
        for (c = 1; c < MAX_WIDTH-1; c++) {
            if (((*next_gen)[r][c] & ~LIVE_MASK) != 
                get_next_gen_cell_live_neighbor_count(r, c))
            {
                INFO("next_gen[%d][%d]=0x%2.2x live neighbor count should be %d\n",
                      r, c, (*next_gen)[r][c], get_next_gen_cell_live_neighbor_count(r, c));
            }
        }
    }
#endif

    // swap the pointers to next_gen and curr_gen
    SWAP(curr_gen, next_gen);
}

// - - - - - - - - -  SIM - SUPPORT  - - - - - - - -

static void set_curr_gen_cell_live_neighbor_count(int32_t r, int32_t c)
{
    int32_t live_neighbors = 0;

    if (r > 0 && c > 0 &&                     ((*curr_gen)[r-1][c-1] & LIVE_MASK)) live_neighbors++;
    if (r > 0 &&                              ((*curr_gen)[r-1][c  ] & LIVE_MASK)) live_neighbors++;
    if (r > 0 && c < MAX_WIDTH-1 &&           ((*curr_gen)[r-1][c+1] & LIVE_MASK)) live_neighbors++;
    if (c > 0 &&                              ((*curr_gen)[r  ][c-1] & LIVE_MASK)) live_neighbors++;
    if (c < MAX_WIDTH-1 &&                    ((*curr_gen)[r  ][c+1] & LIVE_MASK)) live_neighbors++;
    if (r < MAX_WIDTH-1 && c > 0 &&           ((*curr_gen)[r+1][c-1] & LIVE_MASK)) live_neighbors++;
    if (r < MAX_WIDTH-1 &&                    ((*curr_gen)[r+1][c  ] & LIVE_MASK)) live_neighbors++;
    if (r < MAX_WIDTH-1 && c < MAX_WIDTH-1 && ((*curr_gen)[r+1][c+1] & LIVE_MASK)) live_neighbors++;

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

#ifdef UNIT_TEST
static int32_t get_next_gen_cell_live_neighbor_count(int32_t r, int32_t c)
{
    int32_t live_neighbors = 0;

    if ((*next_gen)[r-1][c-1] & LIVE_MASK) live_neighbors++;
    if ((*next_gen)[r-1][c  ] & LIVE_MASK) live_neighbors++;
    if ((*next_gen)[r-1][c+1] & LIVE_MASK) live_neighbors++;
    if ((*next_gen)[r  ][c-1] & LIVE_MASK) live_neighbors++;
    if ((*next_gen)[r  ][c+1] & LIVE_MASK) live_neighbors++;
    if ((*next_gen)[r+1][c-1] & LIVE_MASK) live_neighbors++;
    if ((*next_gen)[r+1][c  ] & LIVE_MASK) live_neighbors++;
    if ((*next_gen)[r+1][c+1] & LIVE_MASK) live_neighbors++;

    return live_neighbors;
}
#endif

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

        // clear the event registration table
        sdl_event_init();

        // clear window
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // draw the current generation
        SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
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
        
        // display ctlpane, and register for events ...

        // - row 0:  title:   GAME OF LIFE
        sdl_render_text_font0(&ctlpane,  0, 3,  "GAME OF LIFE", SDL_EVENT_NONE);

        // - row 2:  RUN|CONT|PAUSE   RESTART   RESET 
        if (running == false && gen_count == 0) {
            sdl_render_text_font0(&ctlpane,  2, 0,  "RUN", SDL_EVENT_RUN);
        } else if (running == false && gen_count != 0) {
            sdl_render_text_font0(&ctlpane,  2, 0,  "CONT", SDL_EVENT_RUN);
        } else {
            sdl_render_text_font0(&ctlpane,  2, 0,  "PAUSE", SDL_EVENT_STOP);
        }
        if (gen_count != 0) {
            sdl_render_text_font0(&ctlpane,  2, 7,  "RESTART", SDL_EVENT_RESTART);
        }
        sdl_render_text_font0(&ctlpane,  2, 16,  "RESET", SDL_EVENT_RESET);

        // - row 4:  SLOW   FAST   n
        sdl_render_text_font0(&ctlpane,  4, 0,  "SLOW", SDL_EVENT_SLOW);
        sdl_render_text_font0(&ctlpane,  4, 7,  "FAST", SDL_EVENT_FAST);
        sprintf(str, "%d", run_speed);
        sdl_render_text_font0(&ctlpane,  4, 16, str, SDL_EVENT_NONE);

        // - row 6:  ZOOM+  ZOOM-
        sdl_render_text_font0(&ctlpane,  6, 0,  "ZOOM+", SDL_EVENT_SIMPANE_ZOOM_IN);
        sdl_render_text_font0(&ctlpane,  6, 7,  "ZOOM-", SDL_EVENT_SIMPANE_ZOOM_OUT);
        sprintf(str, "%d", width);
        sdl_render_text_font0(&ctlpane,  6, 16, str, SDL_EVENT_NONE);

        // - row 8:  RUNNING|PAUSED|RESET   nnnn     rrr,ccc
        if (running) {
            sprintf(str, "RUNNING %d", gen_count);
        } else if (gen_count > 0) {
            sprintf(str, "PAUSED %d", gen_count);
        } else {
            sprintf(str, "RESET");
        }
        sdl_render_text_font0(&ctlpane,  8, 0, str, SDL_EVENT_NONE);
        sprintf(str, "%d,%d", ctr_row-MAX_WIDTH/2, ctr_col-MAX_WIDTH/2);
        sdl_render_text_font0(&ctlpane, 8, 16, str, SDL_EVENT_NONE);

        // - row 10: PARAMS ...
        // - row 11: - INIT   n
        sdl_render_text_font0(&ctlpane, 10, 0, "PARAMS ...", SDL_EVENT_SELECT_PARAMS);
        sprintf(str, "INIT%d", param_init);
        sdl_render_text_font0(&ctlpane, 11, 0, str, SDL_EVENT_NONE);

        // - row N:  HELP   BACK
        sdl_render_text_font0(&ctlpane, -1, 0, "HELP", SDL_EVENT_HELP);
        sdl_render_text_font0(&ctlpane, -1,-5, "BACK", SDL_EVENT_BACK);

        // - register for pan and zoom events
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
            display_sim_reset();
            sim_init();
            break;
        case SDL_EVENT_RESTART:
            sim_init();
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
            while (tmp_col > square_width) {
                tmp_col -= square_width;
                ctr_col++;
            } 
            while (tmp_col < -square_width) {
                tmp_col += square_width;
                ctr_col--;
            }

            tmp_row -= (event->mouse_motion.delta_y);
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

// note: r,c args are relative to top left corner of display, and not
//       the usual indices into curr_gen or next_gen
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

static void display_sim_reset(void)
{
    width     = DEFAULT_WIDTH;
    run_speed = DEFAULT_RUN_SPEED;
    ctr_row   = MAX_WIDTH/2;
    ctr_col   = MAX_WIDTH/2;
}

// - - - - - - - - -  DISPLAY : SELECT_PARAMS  - - - - - - - - - - - - - - - -

static int32_t display_select_params(int32_t curr_display, int32_t last_display)
{
    char cur_s1[100], ret_s1[100];

    // get new value strings for the params
    sprintf(cur_s1, "%d", param_init);
    sdl_display_get_string(1, "INIT", cur_s1, ret_s1);

    // scan returned string(s)
    sscanf(ret_s1, "%d", &param_init);

#if 0
    // xxx constrain values to their allowed range
    if (param_init < 0) param_init = 0;
    if (param_init > 100) param_init = 100;
#endif

    // re-init with the new params
    display_sim_reset();
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
