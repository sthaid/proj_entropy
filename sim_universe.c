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
#include "sim_universe_help.h"

//
// defines
//

#define MB 0x100000

#define MAX_THREAD 16

#define SDL_EVENT_STOP               (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN                (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_SLOW               (SDL_EVENT_USER_START + 2)
#define SDL_EVENT_FAST               (SDL_EVENT_USER_START + 3)
#define SDL_EVENT_RESET              (SDL_EVENT_USER_START + 9)
#define SDL_EVENT_SELECT_PARAMS      (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_SUSPEND_EXPANSION  (SDL_EVENT_USER_START + 11)
#define SDL_EVENT_HELP               (SDL_EVENT_USER_START + 12)
#define SDL_EVENT_BACK               (SDL_EVENT_USER_START + 13)
#define SDL_EVENT_SIMPANE_ZOOM_IN    (SDL_EVENT_USER_START + 20)
#define SDL_EVENT_SIMPANE_ZOOM_OUT   (SDL_EVENT_USER_START + 21)

#define DELTA_TIME                  100          // 100,000 years       units = 1,000 years 

#define DEFAULT_DISPLAY_WIDTH       30000        // 3 billion LY        units = 100,000 LY
#ifndef ANDROID
#define DEFAULT_MAX_PARTICLE        100000       // 100,000             Linux
#else
#define DEFAULT_MAX_PARTICLE        10000        // 10,000              Android
#endif
#define DEFAULT_INITIAL_TIME        1            // 1 thousand years    units = 1,000 years
#define DEFAULT_INITIAL_AVG_SPEED   1000         // 0.1C                units = 0.0001 C

#define MAX_TIME                    300000000    // 30,000 billion LY
#define MAX_AVG_SPEED               3000         // 0.3C

#define STATE_STOP               0
#define STATE_RUN                1
#define STATE_TERMINATE_THREADS  9

#define STATE_STR(s) \
    ((s) == STATE_STOP              ? "STOP"              : \
     (s) == STATE_RUN               ? "RUN"               : \
     (s) == STATE_TERMINATE_THREADS ? "TERMINATE_THREADS"   \
                                    : "????")

#define MIN_RUN_SPEED     1
#define MAX_RUN_SPEED     9
#define DEFAULT_RUN_SPEED MAX_RUN_SPEED
#define RUN_SPEED_SLEEP_TIME (392 * ((1 << (MAX_RUN_SPEED-run_speed)) - 1))

#define DISPLAY_NONE          0
#define DISPLAY_TERMINATE     1
#define DISPLAY_SIMULATION    2
#define DISPLAY_SELECT_PARAMS 3
#define DISPLAY_HELP          4

//
// typedefs
//

typedef struct {
    int32_t x;    // -100000000 to 1000000000 == -current_radius to +current_radius
    int32_t y;
    int32_t xv;   // .0001 C  (C = 10000)
    int32_t yv;
    int32_t r;    // -100000000 to 1000000000
    int32_t rva_color;
} particle_t;

typedef struct {
    // params
    int32_t  max_particle;
    int32_t  initial_time;      // 1000 years
    int32_t  initial_avg_speed; // .0001 C

    // simulation
    int32_t   current_time;             // 1000 years
    double    current_radius_as_double; // 100000 LY
    double    current_radius_as_int32;  // 100000 LY
    particle_t particle[0];
} sim_t;

//
// variables
//

static sim_t   * sim;

static int32_t   state; 
static int32_t   display_width;  // units 100,000 LY
static int32_t   run_speed;
static bool      suspend_expansion;

static int32_t   perf_start_sim_current_time;
static int64_t   perf_start_wall_time;

static int32_t   max_thread;
static pthread_t thread_id[MAX_THREAD];

// 
// prototypes
//

int32_t sim_universe_init(int32_t max_particle, int32_t initial_time, int32_t initial_avg_speed);
void sim_universe_terminate(void);
void sim_universe_display(void);
int32_t sim_universe_display_simulation(int32_t curr_display, int32_t last_display);
int32_t sim_universe_display_select_params(int32_t curr_display, int32_t last_display);
int32_t sim_universe_display_help(int32_t curr_display, int32_t last_display);
void * sim_universe_thread(void * cx);
double  sim_universe_compute_radius(int32_t time);
int32_t sim_universe_barrier(int32_t thread_id);

// -----------------  MAIN  -----------------------------------------------------

void sim_universe(void) 
{
    display_width     = DEFAULT_DISPLAY_WIDTH;
    run_speed         = DEFAULT_RUN_SPEED;
    suspend_expansion = false;

    sim_universe_init(DEFAULT_MAX_PARTICLE, DEFAULT_INITIAL_TIME, DEFAULT_INITIAL_AVG_SPEED);

    sim_universe_display();

    sim_universe_terminate();
}

// -----------------  INIT & TERMINATE  -----------------------------------------

int32_t sim_universe_init(
    int32_t max_particle,
    int32_t initial_time,
    int32_t initial_avg_speed)
{
    #define SIM_SIZE(n) (sizeof(sim_t) + (n) * sizeof(particle_t))

    int32_t i;

    // print info msg
    INFO("initialize start: max_particle=%d initial_time=%d initial_avg_speed=%d\n",
         max_particle, initial_time, initial_avg_speed);

    // terminate threads
    state = STATE_TERMINATE_THREADS;
    for (i = 0; i < max_thread; i++) {
        if (thread_id[i]) {
            pthread_join(thread_id[i], NULL);
            thread_id[i] = 0;
        }
    }
    max_thread = 0;

    // free allocations 
    free(sim);
    sim = NULL;

    // allocate sim
    while (true) {
        sim = calloc(1, SIM_SIZE(max_particle));
        if (sim != NULL) {
            break;
        }
        ERROR("alloc sim failed, max_particle=%d size=%d MB, retrying with max_particle=%d\n",
              max_particle, (int32_t)(SIM_SIZE(max_particle)/MB), max_particle/2);
        max_particle /= 2;
    }

    // initialize sim
    sim->max_particle = max_particle;
    sim->initial_time = initial_time;
    sim->initial_avg_speed = initial_avg_speed;
    sim->current_time = initial_time;
    sim->current_radius_as_double = sim_universe_compute_radius(initial_time);
    sim->current_radius_as_int32 = sim_universe_compute_radius(initial_time);

    for (i = 0; i < max_particle; i++) {
        particle_t * p = &sim->particle[i];
        float  direction, speed;

        while (true) {
            int64_t temp;
            p->x = (double)random()/RAND_MAX * 2000000000 - 1000000000;
            p->y = (double)random()/RAND_MAX * 2000000000 - 1000000000;
            temp = (int64_t)p->x*p->x + (int64_t)p->y*p->y;
            if (temp > 1000000000000000000) {
                continue;
            }
            p->r = sqrt(temp);
            break;
        }

        speed      = random_triangular(-3*initial_avg_speed, 3*initial_avg_speed);
        direction  = ((double)random()/RAND_MAX) * (2.0 * M_PI);
        p->xv      = speed * cosf(direction);
        p->yv      = speed * sinf(direction);

        p->rva_color = GREEN;
    }

    // init control variables
    state = STATE_STOP;

    // create worker threads
    max_thread = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    if (max_thread == 0) {
        max_thread = 1;
    } else if (max_thread > MAX_THREAD) {
        max_thread = MAX_THREAD;
    }
    INFO("max_thread=%dd\n", max_thread);
    for (i = 0; i < max_thread; i++) {
        pthread_create(&thread_id[i], NULL, sim_universe_thread, (void*)(long)i);
    }

    // success
    INFO("initialize complete\n");
    return 0;
}

void sim_universe_terminate(void)
{
    int32_t i;

    // print info msg
    INFO("terminate start\n");

    // terminate threads
    state = STATE_TERMINATE_THREADS;
    for (i = 0; i < max_thread; i++) {
        if (thread_id[i]) {
            pthread_join(thread_id[i], NULL);
            thread_id[i] = 0;
        }
    }
    max_thread = 0;

    // free allocations 
    free(sim);
    sim = NULL;

    // done
    INFO("terminate complete\n");
}

// -----------------  DISPLAY  --------------------------------------------------

void sim_universe_display(void)
{
    int32_t last_display = DISPLAY_NONE;
    int32_t curr_display = DISPLAY_SIMULATION;
    int32_t next_display = DISPLAY_SIMULATION;

    while (true) {
        switch (curr_display) {
        case DISPLAY_SIMULATION:
            next_display = sim_universe_display_simulation(curr_display, last_display);
            break;
        case DISPLAY_SELECT_PARAMS:
            next_display = sim_universe_display_select_params(curr_display, last_display);
            break;
        case DISPLAY_HELP:
            next_display = sim_universe_display_help(curr_display, last_display);
            break;
        }

        if (sdl_quit || next_display == DISPLAY_TERMINATE) {
            break;
        }

        last_display = curr_display;
        curr_display = next_display;
    }
}

int32_t sim_universe_display_simulation(int32_t curr_display, int32_t last_display)
{
    SDL_Rect      ctlpane;
    SDL_Rect      simpane;
    int32_t       simpane_width;
    sdl_event_t * event;
    int32_t       next_display = -1;

    //
    // loop until next_display has been set
    //

    while (next_display == -1) {
        //
        // short delay
        //

        usleep(5000);

        //
        // init
        //

        if (sdl_win_width > sdl_win_height) {
            simpane_width = sdl_win_height;
            SDL_INIT_PANE(simpane,
                          0, 0,                                             // x, y
                          simpane_width, simpane_width);                    // w, h
            SDL_INIT_PANE(ctlpane,
                          simpane_width, 0,                                 // x, y
                          sdl_win_width-simpane_width, sdl_win_height);     // w, h
        } else {
            simpane_width = sdl_win_width;
            SDL_INIT_PANE(simpane, 0, 0, simpane_width, simpane_width);
            SDL_INIT_PANE(ctlpane, 0, 0, 0, 0);
        }
        sdl_event_init();

        //
        // clear window
        //

        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        //
        // draw particles
        //

        #define MAX_POINTS 1000
        SDL_Point points[MAX_COLOR][MAX_POINTS];
        int32_t   max_points[MAX_COLOR];
        int32_t   k1, k2, i;
        int32_t   disp_x, disp_y, disp_color;
        int64_t   current_radius_as_int64;

        bzero(max_points, sizeof(max_points));
        current_radius_as_int64 = sim->current_radius_as_int32;
        k1 = 1000000000 / simpane_width;
        k2 = (simpane_width * current_radius_as_int64 / display_width) -
             (simpane_width / 2);

        for (i = 0; i < sim->max_particle; i++) {
            particle_t * p = &sim->particle[i];

            disp_x = (p->x + 1000000000) / k1 * current_radius_as_int64 / display_width - k2; 
            if (disp_x < 0 || disp_x >= simpane_width) {
                continue;
            }
            disp_y = (p->y + 1000000000) / k1 * current_radius_as_int64 / display_width - k2;
            if (disp_y < 0|| disp_y >= simpane_width) {
                continue;
            }
            disp_color = p->rva_color;

            points[disp_color][max_points[disp_color]].x = disp_x;
            points[disp_color][max_points[disp_color]].y = disp_y;
            max_points[disp_color]++;

            if (max_points[disp_color] == MAX_POINTS) {
                SDL_SetRenderDrawColor(sdl_renderer, 
                                       sdl_pixel_r[disp_color],
                                       sdl_pixel_g[disp_color],
                                       sdl_pixel_b[disp_color],
                                       SDL_ALPHA_OPAQUE);
                SDL_RenderDrawPoints(sdl_renderer, points[disp_color], max_points[disp_color]);
                max_points[disp_color] = 0;
            }
        }

        for (disp_color = 0; disp_color < MAX_COLOR; disp_color++) {
            if (max_points[disp_color] > 0) {
                SDL_SetRenderDrawColor(sdl_renderer, 
                                       sdl_pixel_r[disp_color],
                                       sdl_pixel_g[disp_color],
                                       sdl_pixel_b[disp_color],
                                       SDL_ALPHA_OPAQUE);
                SDL_RenderDrawPoints(sdl_renderer, points[disp_color], max_points[disp_color]);
                max_points[disp_color] = 0;
            }
        }

        //
        // clear ctlpane
        //

        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(sdl_renderer, &ctlpane);

        //
        // display controls
        //

        char rs_str[20], suspend_str[20];
        sprintf(rs_str, "%3d", run_speed);
        sprintf(suspend_str, "%s", suspend_expansion ? "RES" : "SUS");

        sdl_render_text_font0(&ctlpane,  0, 3,  "UNIVERSE",  SDL_EVENT_NONE);
        sdl_render_text_font0(&ctlpane,  2, 0,  "RUN",       SDL_EVENT_RUN);
        sdl_render_text_font0(&ctlpane,  2, 8,  "STOP",      SDL_EVENT_STOP);
        sdl_render_text_font0(&ctlpane,  2, 16, suspend_str, SDL_EVENT_SUSPEND_EXPANSION);
        sdl_render_text_font0(&ctlpane,  4, 0,  "SLOW",      SDL_EVENT_SLOW);
        sdl_render_text_font0(&ctlpane,  4, 8,  "FAST",      SDL_EVENT_FAST);
        sdl_render_text_font0(&ctlpane,  4, 16, rs_str,      SDL_EVENT_NONE);
        sdl_render_text_font0(&ctlpane,  6, 0,  "RESET",     SDL_EVENT_RESET);
        sdl_render_text_font0(&ctlpane,  6, 8,  "PARAMS",    SDL_EVENT_SELECT_PARAMS);
        sdl_render_text_font0(&ctlpane, 18, 0,  "HELP",      SDL_EVENT_HELP);
        sdl_render_text_font0(&ctlpane, 18,15,  "BACK",      SDL_EVENT_BACK);

        int32_t col = SDL_PANE_COLS(&simpane,0)-2;
        sdl_render_text_font0(&simpane,  0, col,  "+", SDL_EVENT_SIMPANE_ZOOM_IN);
        sdl_render_text_font0(&simpane,  2, col,  "-", SDL_EVENT_SIMPANE_ZOOM_OUT);

        //
        // display status lines
        //

        // - state, current_time
        char str[100];
        sprintf(str, "%-6s %0.3lf BYR", STATE_STR(state), (double)sim->current_time*1000/1E9);
        sdl_render_text_font0(&ctlpane,  8, 0, str, SDL_EVENT_NONE);

        // - current_radius
        sprintf(str, "RADIUS %0.3lf BLY", sim->current_radius_as_double*100000/1E9);
        sdl_render_text_font0(&ctlpane,  9, 0, str, SDL_EVENT_NONE);

        // - performance rate
        if (state == STATE_RUN && !suspend_expansion) {
            double perf = (double)(sim->current_time - perf_start_sim_current_time) / (microsec_timer() - perf_start_wall_time);
            //printf("current_time %d rsct %d \n", sim->current_time, perf_start_sim_current_time);
            sprintf(str, "SIM_RT %0.3lf BYR/S", perf);
            sdl_render_text_font0(&ctlpane, 10, 0, str, SDL_EVENT_NONE);
        }

        // - params
        sdl_render_text_font0(&ctlpane, 12, 0, "PARAMS ...", SDL_EVENT_NONE);
        sprintf(str, "N_PART=%d", sim->max_particle);    
        sdl_render_text_font0(&ctlpane, 13, 0, str, SDL_EVENT_NONE);
        sprintf(str, "START =%0.6lf BYR", (double)sim->initial_time*1E3/1E9);
        sdl_render_text_font0(&ctlpane, 14, 0, str, SDL_EVENT_NONE);
        sprintf(str, "AVGSPD=%0.3lf C", (double)sim->initial_avg_speed*1E-4);
        sdl_render_text_font0(&ctlpane, 15, 0, str, SDL_EVENT_NONE);

        // - window width
        sprintf(str, "WIDTH %0.3lf BLY", (double)display_width*1E5/1E9);
        sdl_render_text_font0(&simpane,  0, 0, str, SDL_EVENT_NONE);

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
        if (event->event != SDL_EVENT_NONE) {
            sdl_play_event_sound();
        }
        switch (event->event) {
        case SDL_EVENT_RUN:
            state = STATE_RUN;
            perf_start_sim_current_time = sim->current_time;
            perf_start_wall_time = microsec_timer();
            break;
        case SDL_EVENT_STOP:
            state = STATE_STOP;
            break;
        case SDL_EVENT_SLOW:
            if (run_speed > MIN_RUN_SPEED) {
                run_speed--;
            }
            perf_start_sim_current_time = sim->current_time;
            perf_start_wall_time = microsec_timer();
            break;
        case SDL_EVENT_FAST:
            if (run_speed < MAX_RUN_SPEED) {
                run_speed++;
            }
            perf_start_sim_current_time = sim->current_time;
            perf_start_wall_time = microsec_timer();
            break;
        case SDL_EVENT_RESET:
            state = STATE_STOP;
            sim_universe_init(sim->max_particle,
                              sim->initial_time,
                              sim->initial_avg_speed);
            break;
        case SDL_EVENT_SELECT_PARAMS:
            state = STATE_STOP;
            next_display = DISPLAY_SELECT_PARAMS;
            break; 
        case SDL_EVENT_SUSPEND_EXPANSION:
            suspend_expansion = !suspend_expansion;
            perf_start_sim_current_time = sim->current_time;
            perf_start_wall_time = microsec_timer();
            break;
        case SDL_EVENT_HELP: 
            state = STATE_STOP;
            next_display = DISPLAY_HELP;
            break;
        case SDL_EVENT_SIMPANE_ZOOM_OUT:
            if (display_width == 1000000000) {
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
        default:
            break;
        }
    }

    //
    // return next_display
    //

    return next_display;
}

int32_t sim_universe_display_select_params(int32_t curr_display, int32_t last_display)
{
    int32_t max_particle        = sim->max_particle;
    int32_t initial_time        = sim->initial_time;
    int32_t initial_avg_speed   = sim->initial_avg_speed;
    double  val;

    char cur_s1[100], cur_s2[100], cur_s3[100];
    char ret_s1[100], ret_s2[100], ret_s3[100];

    // get new value strings for the params
    sprintf(cur_s1, "%d",    max_particle);
    sprintf(cur_s2, "%0.6lf", (double)initial_time*1E3/1E9) ;   
    sprintf(cur_s3, "%0.3lf", (double)initial_avg_speed*1E-4);
    sdl_display_get_string(3,
                           "N_PART", cur_s1, ret_s1,
                           "START",  cur_s2, ret_s2,
                           "AVGSPD", cur_s3, ret_s3);

    // scan returned strings
    if (sscanf(ret_s1, "%lf", &val) == 1) {
        max_particle = val;
        if (max_particle < 1000) max_particle = 1000;
        if (max_particle > 10000000) max_particle = 10000000;
    }
    if (sscanf(ret_s2, "%lf", &val) == 1) {
        initial_time = val / (1E3/1E9);
        if (initial_time < 1) initial_time = 1;
        if (initial_time > MAX_TIME) initial_time = MAX_TIME;
    }
    if (sscanf(ret_s3, "%lf", &val) == 1) {
        initial_avg_speed = val / 1E-4;
        if (initial_avg_speed < 0) initial_avg_speed = 0;
        if (initial_avg_speed > MAX_AVG_SPEED) initial_avg_speed = MAX_AVG_SPEED;
    }

    // re-init with new params
    sim_universe_terminate();
    sim_universe_init(max_particle, initial_time, initial_avg_speed);

    // return next_display
    return sdl_quit ? DISPLAY_TERMINATE : DISPLAY_SIMULATION;
}

int32_t sim_universe_display_help(int32_t curr_display, int32_t last_display)
{
    // display the help text
    sdl_display_text(sim_universe_help);
    
    // return next_display
    return sdl_quit ? DISPLAY_TERMINATE : DISPLAY_SIMULATION;
}

// -----------------  THREAD  ---------------------------------------------------

void * sim_universe_thread(void * cx)
{
    #define BATCH_SIZE 1000

    int32_t         thread_id = (long)cx;
    int32_t         local_state;

    static int64_t  batch_idx;
    static int64_t  batch_count;

    INFO("thread starting, thread_id=%d\n", thread_id);
    if (thread_id == 0) {
        batch_idx  = 0;
        batch_count = 0;
    }

    while (true) {
        // barrier
        local_state = sim_universe_barrier(thread_id);

        // process based on local_state
        if (local_state == STATE_RUN) {
            int32_t batch_start, batch_end, num_in_batch, i;
            int32_t current_time_next;
            int32_t x_next, y_next, r_next;
            int32_t rva_color_next;
            int32_t delta_r, rva;
            double  current_radius_next, current_radius_expf;

            static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

            // determine next values for time and radius,
            // and determine the radius expansion factor
            if (!suspend_expansion) {
                current_time_next = sim->current_time + DELTA_TIME;
            } else {
                current_time_next = sim->current_time;
            }
            current_radius_next = sim_universe_compute_radius(current_time_next);
            current_radius_expf = (double)(current_radius_next - sim->current_radius_as_double) / sim->current_radius_as_double;

            while (true) {
                // get a batch of particle to process
                pthread_mutex_lock(&mutex);
                if (batch_idx / sim->max_particle > batch_count) {
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                batch_start = batch_idx % sim->max_particle;
                num_in_batch = batch_start + BATCH_SIZE > sim->max_particle ? sim->max_particle - batch_start : BATCH_SIZE;
                batch_end = batch_start + num_in_batch;
                batch_idx += num_in_batch;
                pthread_mutex_unlock(&mutex);

                // loop over the particles in the batch
                for (i = batch_start; i < batch_end; i++) {
                    particle_t * p = &sim->particle[i];

                    // compute next x,y position based on the proper motion
                    // notes:
                    //   x_next = x + xv/10000 * (DELTA_TIME*1000) * (1000000000/(current_radius*100000))
                    //                 ly/yr   *     yrs           *    max-x   /       ly
                    x_next = p->x + p->xv * (1000 * DELTA_TIME) / sim->current_radius_as_int32;
                    y_next = p->y + p->yv * (1000 * DELTA_TIME) / sim->current_radius_as_int32;
                    r_next = sqrt((int64_t)x_next*x_next + (int64_t)y_next*y_next);  

                    // when a particles location exceeds the universe radius then reverse its 
                    // velocity, so it will come back within the radius on subsquent iteration
                    if (r_next > 1000000000) {
                        p->xv = -p->xv;
                        p->yv = -p->yv;
                    }

                    // compute particle's apparent radial velocity, including
                    // - change in position due to proper motion (from above), and 
                    // - expansion of the universe
                    delta_r = (r_next - p->r) + (current_radius_expf * p->r);
                    rva = delta_r * (int64_t)sim->current_radius_as_int32 / (DELTA_TIME * 1000);

                    // convert the apparent radial velocity to color
                    if (rva > 10000) {
                        rva_color_next = PURPLE;
                    } else if (rva > 7500) {
                        rva_color_next = RED;
                    } else if (rva > 5000) {
                        rva_color_next = ORANGE;
                    } else if (rva > 1000) {
                        rva_color_next = YELLOW;
                    } else if (rva > -1000) {
                        rva_color_next = GREEN;
                    } else if (rva > -2000) {
                        rva_color_next = LIGHT_BLUE;
                    } else {
                        rva_color_next = BLUE;
                    }

                    // assign next values to the particle
                    p->x = x_next;
                    p->y = y_next;
                    p->r = r_next;
                    p->rva_color = rva_color_next;
                }
            }

            // barrier
            sim_universe_barrier(thread_id);

            // all particles have been updated, thread 0 does post processing
            if (thread_id == 0) {
                // adjust variables for next simulation cycle
                batch_count++;
                sim->current_time = current_time_next;
                sim->current_radius_as_double = current_radius_next;
                sim->current_radius_as_int32 = current_radius_next;

                // auto stop when current_radius reaches 30,000 BLY
                if (sim->current_radius_as_int32 >= MAX_TIME) {
                    state = STATE_STOP;
                }
                
                // sleep to pace the simulation
                if (RUN_SPEED_SLEEP_TIME > 0) {
                    usleep(RUN_SPEED_SLEEP_TIME);
                }
            }

        } else if (local_state == STATE_STOP) {
            usleep(50000);

        } else {  // local_state == STATE_TERMINATE_THREADS
            break;
        }
    }

    INFO("thread terminating, thread_id=%d\n", thread_id);
    return NULL;
}

double sim_universe_compute_radius(int32_t time)
{
    double r1, r2;

    // Units:
    // - TIME  : Billion Years
    // - R1,R2 : Billion Light Years
    //
    //    TIME          R1         R2          R1+R2
    // -----------    ------      ------      --------
    // time=0 BYR:   0.000000  + 0.000000   = 0.000000
    // time=1 BYR:   5.000000  + 0.227259   = 5.227259
    // time=2 BYR:   7.071068  + 0.557810   = 7.628878
    // time=3 BYR:   8.660254  + 1.038604   = 9.698858
    // time=4 BYR:   10.000000 + 1.737926   = 11.737926
    // time=5 BYR:   11.180340 + 2.755102   = 13.935442
    // time=6 BYR:   12.247449 + 4.234602   = 16.482051
    // time=7 BYR:   13.228757 + 6.386561   = 19.615318
    // time=8 BYR:   14.142136 + 9.516623   = 23.658759
    // time=9 BYR:   15.000000 + 14.069353  = 29.069353
    // time=10 BYR:  15.811388 + 20.691377  = 36.502765
    // time=11 BYR:  16.583124 + 30.323226  = 46.906350
    // time=12 BYR:  17.320508 + 44.332918  = 61.653426
    // time=13 BYR:  18.027756 + 64.710258  = 82.738015
    // time=14 BYR:  18.708287 + 94.349454  = 113.057741
    // time=15 BYR:  19.364917 + 137.460180 = 156.825097
    // time=16 BYR:  20.000000 + 200.165481 = 220.165481
    // time=17 BYR:  20.615528 + 291.371430 = 311.986958
    // time=18 BYR:  21.213203 + 424.032069 = 445.245273
    // time=19 BYR:  21.794495 + 616.989276 = 638.783770

    r1 = 50 * sqrt(time);
    r2 = 5000 * (exp(log(2) * time / 1850000) - 1);

    return r1 + r2;
}

int32_t sim_universe_barrier(int32_t thread_id)
{
    static volatile int32_t count[MAX_THREAD];
    static volatile int32_t go_count;
    static volatile int32_t ret_state;

    int32_t my_count, i;

    // just return if max_thread is 1
    if (max_thread == 1) {
        return state;
    }

    // increment count for calling thread_id, and 
    // store counter value for my thread in variable my_count
    count[thread_id]++;
    my_count = count[thread_id];

    // if thread_id is 0 then
    //   - spin wait for all thread count values to be equal
    //   - set ret_state global, so all threads will return the same state value
    //   - set go_count global, which will release the other threads
    // else
    //   - spin wait for go_count to become set t my_count
    // endif
    if (thread_id == 0) {
        while (true) {
            for (i = 0; i < max_thread; i++) {
                if (count[i] != my_count) {
                    break;
                }
            }
            if (i == max_thread) {
                break;
            }
            sched_yield();
        }
        ret_state = state;
        __sync_synchronize();
        go_count = my_count;
    } else {
        while (my_count != go_count) {
            sched_yield();
        }
    }

    return ret_state;
}        
