// XXX 
//
// - better unit test ??
// - init threads should run one cycle
// - 'reset' ctl not working

// - int64_t
// - improve default values
// - remember last apparanet velocity compontnt due to expansion, if needed
// - more efficient init, cut time down for 10 million particles
// - use multiple threads
// - overflow checks in simulation thread
// - on memory alloc failure, display error, and revert to something that does work

// MAY NEED MORE WORK
// - wrap

// DONE
// - get rid of circles
// - colors, need to rewrite the color code
// - color scaling
// - latch color, or compute expansion better

// NOT DOING THIS
// - delta time controls ??

#include "util.h"

//
// defines
//

#define MB 0x100000

#define MAX_THREAD 16

#define SDL_EVENT_STOP               (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN                (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_RESET              (SDL_EVENT_USER_START + 9)
#define SDL_EVENT_SELECT_PARAMS      (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_HELP               (SDL_EVENT_USER_START + 11)
#define SDL_EVENT_BACK               (SDL_EVENT_USER_START + 12)
#define SDL_EVENT_SIMPANE_ZOOM_IN    (SDL_EVENT_USER_START + 20)
#define SDL_EVENT_SIMPANE_ZOOM_OUT   (SDL_EVENT_USER_START + 21)

#define DEFAULT_MAX_PARTICLE        100000       // 100,000         
#define DEFAULT_INITIAL_RADIUS      100          // 10 million  LY    units = 100,000 LY
#define DEFAULT_INITIAL_AVG_SPEED   1000         // 0.1C              units = 0.0001 C
#define DEFAULT_RADIUS_DOUBLE_INTVL 1000000      // 1 billion years   units = 1,000 years
#define DEFAULT_DELTA_TIME          1000         // 1 million years   units = 1,000 years 
#define DEFAULT_DISPLAY_WIDTH       10000        // 1 billion LY      units = 100,000 LY

#define STATE_STOP               0
#define STATE_RUN                1
#define STATE_TERMINATE_THREADS  9

#define STATE_STR(s) \
    ((s) == STATE_STOP              ? "STOP"              : \
     (s) == STATE_RUN               ? "RUN"               : \
     (s) == STATE_TERMINATE_THREADS ? "TERMINATE_THREADS"   \
                                    : "????")

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
    int32_t  initial_radius;       // 100000 LY
    int32_t  initial_avg_speed;    // .0001 C
    int32_t  radius_double_intvl;  // 1000 years

    // simulation
    int32_t   current_time;        // 1000 years
    int32_t   current_radius;      // 100000 LY
    particle_t particle[0];
} sim_t;

//
// variables
//

static sim_t           * sim;

static int32_t           state; 
static int32_t           delta_time;     // 1000 years
static int32_t           display_width;  // 100000 LY

static int32_t           max_thread;
static pthread_t         thread_id[MAX_THREAD];

// 
// prototypes
//

int32_t sim_universe_init(int32_t max_particle, int32_t initial_radius,
                          int32_t initial_avg_speed, int32_t radius_double_intvl);
void sim_universe_terminate(void);
void sim_universe_display(void);
int32_t sim_universe_display_simulation(int32_t curr_display, int32_t last_display);
int32_t sim_universe_display_select_params(int32_t curr_display, int32_t last_display);
int32_t sim_universe_display_help(int32_t curr_display, int32_t last_display);
void * sim_universe_thread(void * cx);
int32_t sim_universe_barrier(int32_t thread_id);

// -----------------  MAIN  -----------------------------------------------------

void sim_universe(void) 
{
    display_width = DEFAULT_DISPLAY_WIDTH;
    delta_time = DEFAULT_DELTA_TIME;

    sim_universe_init(DEFAULT_MAX_PARTICLE, DEFAULT_INITIAL_RADIUS,
                      DEFAULT_INITIAL_AVG_SPEED, DEFAULT_RADIUS_DOUBLE_INTVL);

    sim_universe_display();

    sim_universe_terminate();
}

// -----------------  INIT & TERMINATE  -----------------------------------------

int32_t sim_universe_init(
    int32_t max_particle,
    int32_t initial_radius,
    int32_t initial_avg_speed,
    int32_t radius_double_intvl)
{
    #define SIM_SIZE(n) (sizeof(sim_t) + (n) * sizeof(particle_t))

    int32_t i;

    // print info msg
    INFO("initialize start: max_particle=%d initial_radius=%d"
         " initial_avg_speed=%d radius_double_intvl=%d\n",
         max_particle, initial_radius, initial_avg_speed, radius_double_intvl);

    // allocate sim
    free(sim);
    sim = calloc(1, SIM_SIZE(max_particle));
    if (sim == NULL) {
        // xxx fake in a dummy sim that is malloced but with small max_particles
        return -1;
    }

    // initialize sim
    // xxx optimize particle init loop
    sim->max_particle        = max_particle;
    sim->initial_radius      = initial_radius;
    sim->initial_avg_speed   = initial_avg_speed;
    sim->radius_double_intvl = radius_double_intvl;

    sim->current_time        = 0;
    sim->current_radius      = initial_radius;

    for (i = 0; i < max_particle; i++) {
        particle_t * p = &sim->particle[i];
        double  direction, distance, speed;

        distance   = sqrt((double)random()/RAND_MAX) * 1000000000;
        direction  = ((double)random()/RAND_MAX) * (2.0 * M_PI);
        p->x       = distance * cos(direction);
        p->y       = distance * sin(direction);
        p->r       = sqrt((int64_t)p->x*p->x + (int64_t)p->y*p->y);

        speed      = random_triangular(-3*initial_avg_speed, 3*initial_avg_speed);
        direction  = ((double)random()/RAND_MAX) * (2.0 * M_PI);
        p->xv      = speed * cos(direction);
        p->yv      = speed * sin(direction);

if (i < 100) {
    printf("SPEED %lf %d %d\n", speed, p->xv, p->yv);
}

        p->rva_color = GREEN;
    }

    // init control variables
    state = STATE_STOP;

    // create worker threads
#ifndef ANDROID
    max_thread = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    if (max_thread == 0) {
        max_thread = 1;
    } else if (max_thread > MAX_THREAD) {
        max_thread = MAX_THREAD;
    }
    max_thread = 1;  // xxx temp, test this later
#else
    max_thread = 1;
#endif
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
        // AAA next
        // xxx add debug timing to periodically print display update cpu time
        //

        #define MAX_POINTS 1000
        SDL_Point points[MAX_COLOR][MAX_POINTS];
        int32_t   max_points[MAX_COLOR];
        int32_t   k1, i;
        int32_t   disp_x, disp_y, disp_color;

        bzero(max_points, sizeof(max_points));
        k1 = 1000000000 / simpane_width;

        for (i = 0; i < sim->max_particle; i++) {
            particle_t * p = &sim->particle[i];

            disp_x  = p->x / k1 * sim->current_radius / display_width;
            if (disp_x < -simpane_width/2 || disp_x > simpane_width/2) {
                continue;
            }
            disp_y  = p->y / k1 * sim->current_radius / display_width;
            if (disp_y < -simpane_width/2 || disp_y > simpane_width/2) {
                continue;
            }
            disp_color = p->rva_color;

            points[disp_color][max_points[disp_color]].x = disp_x + simpane_width / 2;
            points[disp_color][max_points[disp_color]].y = disp_y + simpane_width / 2;
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

        sdl_render_text_font0(&ctlpane,  0, 3,  "UNIVERSE",  SDL_EVENT_NONE);
        sdl_render_text_font0(&ctlpane,  2, 0,  "RUN",       SDL_EVENT_RUN);
        sdl_render_text_font0(&ctlpane,  2, 9,  "STOP",      SDL_EVENT_STOP);
        sdl_render_text_font0(&ctlpane,  4, 0,  "RESET",     SDL_EVENT_RESET);
        sdl_render_text_font0(&ctlpane,  4, 9,  "PARAMS",    SDL_EVENT_SELECT_PARAMS);
        sdl_render_text_font0(&ctlpane, 18, 0,  "HELP",      SDL_EVENT_HELP);
        sdl_render_text_font0(&ctlpane, 18,15,  "BACK",      SDL_EVENT_BACK);

        int32_t col = SDL_PANE_COLS(&simpane,0)-2;
        sdl_render_text_font0(&simpane,  0, col,  "+", SDL_EVENT_SIMPANE_ZOOM_IN);
        sdl_render_text_font0(&simpane,  2, col,  "-", SDL_EVENT_SIMPANE_ZOOM_OUT);

        //
        // display status lines
        //

        // - state & time
        char str[100];
        sprintf(str, "%-4s %0.3lf BYR", STATE_STR(state), (double)sim->current_time*1000/1E9);
        sdl_render_text_font0(&ctlpane,  6, 0, str, SDL_EVENT_NONE);

#if 0
        // - delta_time
        sprintf(str, "DELTA %d YR", delta_time*1000);
        sdl_render_text_font0(&ctlpane,  7, 0, str, SDL_EVENT_NONE);
#endif

        // - params
        sdl_render_text_font0(&ctlpane, 12, 0, "PARAMS ...", SDL_EVENT_NONE);
        sprintf(str, "N_PART = %d", sim->max_particle);    
        sdl_render_text_font0(&ctlpane, 13, 0, str, SDL_EVENT_NONE);
        sprintf(str, "RADIUS = %0.3lf BLY", (double)sim->initial_radius*1E5/1E9);
        sdl_render_text_font0(&ctlpane, 14, 0, str, SDL_EVENT_NONE);
        sprintf(str, "SPEED  = %0.3lf C", (double)sim->initial_avg_speed*1E-4);
        sdl_render_text_font0(&ctlpane, 15, 0, str, SDL_EVENT_NONE);
        sprintf(str, "DOUBLE = %0.3lf BYR", (double)sim->radius_double_intvl*1E3/1E9);
        sdl_render_text_font0(&ctlpane, 16, 0, str, SDL_EVENT_NONE);

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
            break;
        case SDL_EVENT_STOP:
            state = STATE_STOP;
            break;
        case SDL_EVENT_RESET: {
            state = STATE_STOP;
            // xxx preserve if args are 0
            // sim_universe_terminate();
            // sim_universe_init(max_particle, sim_width);
            break; }
        case SDL_EVENT_SELECT_PARAMS:
            state = STATE_STOP;
            next_display = DISPLAY_SELECT_PARAMS;
            break; 
        case SDL_EVENT_HELP: 
            state = STATE_STOP;
            next_display = DISPLAY_HELP;
            break;
        case SDL_EVENT_SIMPANE_ZOOM_OUT:
            if ((display_width % 3) != 0) {
                display_width = display_width * 3;
            } else {
                display_width = display_width / 3 * 10;
            }
            if (display_width > 10000000) {  // xxx bigger ?
                display_width = 10000000;
            }
            break;
        case SDL_EVENT_SIMPANE_ZOOM_IN:
            if ((display_width % 3) != 0) {
                display_width = display_width / 10 * 3;
            } else {
                display_width = display_width / 3;
            }
            if (display_width < 30) {
                display_width = 30;
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
    int32_t initial_radius      = sim->initial_radius;
    int32_t initial_avg_speed   = sim->initial_avg_speed;
    int32_t radius_double_intvl = sim->radius_double_intvl;
    double  val;

    char cur_s1[100], cur_s2[100], cur_s3[100], cur_s4[100];
    char ret_s1[100], ret_s2[100], ret_s3[100], ret_s4[100];

    // get new value strings for the params
    sprintf(cur_s1, "%d",    max_particle);
    sprintf(cur_s2, "%0.3lf", (double)initial_radius*1E5/1E9) ;   
    sprintf(cur_s3, "%0.3lf", (double)initial_avg_speed*1E-4);
    sprintf(cur_s4, "%0.3lf", (double)radius_double_intvl*1E3/1E9);
    sdl_display_get_string(4,
                           "N_PART", cur_s1, ret_s1,
                           "RADIUS", cur_s2, ret_s2,
                           "SPEED",  cur_s3, ret_s3,
                           "DOUBLE", cur_s4, ret_s4);

    // scan returned strings
    // xxx check ranges
    if (sscanf(ret_s1, "%lf", &val) == 1) {
        max_particle = val;
    }
    if (sscanf(ret_s2, "%lf", &val) == 1) {
        initial_radius = val / (1E5/1E9);
    }
    if (sscanf(ret_s3, "%lf", &val) == 1) {
        initial_avg_speed = val / 1E-4;
    }
    if (sscanf(ret_s4, "%lf", &val) == 1) {
        radius_double_intvl = val / (1E3/1E9);
    }

    // re-init with new params
    sim_universe_terminate();
    sim_universe_init(max_particle, initial_radius, initial_avg_speed, radius_double_intvl);

    // return next_display
    return sdl_quit ? DISPLAY_TERMINATE : DISPLAY_SIMULATION;
}

int32_t sim_universe_display_help(int32_t curr_display, int32_t last_display)
{
    // display the help text0
    sdl_display_text("xxx HELP xxx", NULL);
    
    // return next_display
    return sdl_quit ? DISPLAY_TERMINATE : DISPLAY_SIMULATION;
}

// -----------------  THREAD  ---------------------------------------------------

void * sim_universe_thread(void * cx)
{
    #define BATCH_SIZE 100 

    int32_t         thread_id = (long)cx;
    int32_t         local_state;

    static int64_t  batch_idx;
    static int64_t  batch_count;

    INFO("thread starting, thread_id=%d\n", thread_id);
    if (thread_id == 0) {
        batch_idx  = 0;
        batch_count = 0;
    }

    int32_t last_current_radius_expansion = 0;

    while (true) {
        // barrier
        local_state = sim_universe_barrier(thread_id);

        // process based on local_state
        if (local_state == STATE_RUN) {
            int32_t batch_start, batch_end, num_in_batch, i;
            int32_t current_time_next, current_radius_next;
            int32_t x_next, y_next, r_next;
            int32_t rva_color_next;
            int32_t current_radius_expansion, delta_r, rva;

            static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

            // determine new value for current_radius
            current_time_next = sim->current_time + delta_time;
            current_radius_next = sim->initial_radius * exp( log(2) * current_time_next / sim->radius_double_intvl );

            current_radius_expansion = current_radius_next - sim->current_radius;

            if (current_radius_expansion < last_current_radius_expansion) {
                current_radius_expansion = last_current_radius_expansion;
            }
            last_current_radius_expansion = current_radius_expansion;

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

                    // XXX 
                    // - make this a separate 'inline' routine
                    // - add unit test support
                    // - check for overflows
                    // - profiling
                    // - avoid use of sqrt and int64_t
                    // - delete the commented out prints

                    // Step 1: compute next x,y position based on the proper motion
                    // Notes:
                    //   x_next = x + xv/10000 * (delta_time*1000) * (1000000000/(current_radius*100000))
                    //                 ly/yr   *     yrs           *    max-x   /       ly

                    x_next = p->x + p->xv * 1000 * delta_time / sim->current_radius;
                    y_next = p->y + p->yv * 1000 * delta_time / sim->current_radius;
                    r_next = sqrt((int64_t)x_next*x_next + (int64_t)y_next*y_next);  

                    // XXX comment
                    if (r_next > sim->current_radius) {
                        p->xv = -p->xv;
                        p->yv = -p->yv;
                    }

                    // Step 2: compute particle's apparent radial velocity, including
                    // - change in position from above, and 
                    // - expansion of the universe

                    delta_r = (r_next - p->r) + 
                              ((int64_t)current_radius_expansion * p->r / sim->current_radius);
                    rva = (int64_t)delta_r * sim->current_radius / (delta_time * 1000);

                    // convert the apparent radial velocity to color
                    // xxx use (int32_t)(.1 * C), etc
                    if (rva > 10000) {
                        rva_color_next = PURPLE;
                    } else if (rva > 7500) {
                        rva_color_next = RED;
                    } else if (rva > 5000) {
                        rva_color_next = ORANGE;
                    } else if (rva > 2500) {
                        rva_color_next = YELLOW;
                    } else if (rva > -2500) {
                        rva_color_next = GREEN;
                    } else if (rva > -5000) {
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
                batch_count++;
                sim->current_time = current_time_next;
                sim->current_radius = current_radius_next;
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

#ifndef ANDROID
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
            asm volatile("pause\n": : :"memory");
        }
        ret_state = state;
        asm volatile("" ::: "memory");
        go_count = my_count;
    } else {
        while (my_count != go_count) {
            asm volatile("pause\n": : :"memory");
        }
    }

    return ret_state;
}        
#else
int32_t sim_universe_barrier(int32_t thread_id)
{
    // not needed on Android becuase max_thread is forced to 1
    return state;
}        
#endif