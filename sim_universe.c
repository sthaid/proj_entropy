#include "util.h"

//
// defines
//

#define MB 0x100000

#define MAX_THREAD 16

#define SDL_EVENT_STOP           (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN            (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_RESET          (SDL_EVENT_USER_START + 9)
#define SDL_EVENT_SELECT_PARAMS  (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_HELP           (SDL_EVENT_USER_START + 11)
#define SDL_EVENT_BACK           (SDL_EVENT_USER_START + 12)
// xxx delta time

#define DEFAULT_MAX_PARTICLE        1000
#define DEFAULT_INITIAL_WIDTH       1E9
#define DEFAULT_INITIAL_AVG_SPEED   3E7
#define DEFAULT_WIDTH_DOUBLE_YEARS  2E9

//xxx #define MAX_MAX_PARTICLE      10000
//xxx #define MIN_MAX_PARTICLE      1

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
    __int128_t x;        // m
    __int128_t y;        
    int64_t    x_speed;  // m/sec
    int64_t    y_speed;
} particle_t;

typedef struct {
    // params
    int64_t max_particle;
    double  initial_radius;       // m
    double  initial_avg_speed;    // m/sec  xxx rename initial_avg_speed
    double  radius_double_years;  // year

    // simulation
    double  curr_year;           // year 
    particle_t particle[0];
} sim_t;

//
// variables
//

static sim_t           * sim;

static int32_t           state; 
static double            display_width;       // m

static int32_t           max_thread;
static pthread_t         thread_id[MAX_THREAD];
static pthread_barrier_t barrier;

// 
// prototypes
//

int32_t sim_universe_init(int64_t max_particle, double initial_radius,
                          double initial_avg_speed, double radius_double_years);
void sim_universe_terminate(void);
void sim_universe_display(void);
int32_t sim_universe_display_simulation(int32_t curr_display, int32_t last_display);
int32_t sim_universe_display_select_params(int32_t curr_display, int32_t last_display);
int32_t sim_universe_display_help(int32_t curr_display, int32_t last_display);
void * sim_universe_thread(void * cx);

// -----------------  MAIN  -----------------------------------------------------

void sim_universe(void) 
{
    sim_universe_init(DEFAULT_MAX_PARTICLE, DEFAULT_INITIAL_WIDTH,
                      DEFAULT_INITIAL_AVG_SPEED, DEFAULT_WIDTH_DOUBLE_YEARS);

    sim_universe_display();

    sim_universe_terminate();
}

// -----------------  INIT & TERMINATE  -----------------------------------------

int32_t sim_universe_init(
    int64_t max_particle,
    double initial_radius,
    double initial_avg_speed,
    double radius_double_years)
{
    #define SIM_SIZE(n) (sizeof(sim_t) + (n) * sizeof(particle_t))

    int32_t i;

    // print info msg
    INFO("initialize start, max_particle=%"PRId64" initial_radius=%G"
         " initial_avg_speed=%G radius_double_years=%G\n",
         max_particle, initial_radius, initial_avg_speed, radius_double_years);

    // allocate sim
    free(sim);
    sim = calloc(1, SIM_SIZE(max_particle));
    if (sim == NULL) {
        // xxx test this path
        // xxx fake in a dummy sim that is malloced but with small max_particles
        return -1;
    }

    // initialize sim
    sim->max_particle       = max_particle;
    sim->initial_radius      = initial_radius;
    sim->initial_avg_speed  = initial_avg_speed;   // XXX limit on this  0.14
    sim->radius_double_years = radius_double_years;

    sim->curr_year          = 0;
    for (i = 0; i < max_particle; i++) {
        particle_t * p = &sim->particle[i];
        double  direction, distance, speed;

        distance   = ((double)random()/RAND_MAX) * (sim->initial_radius);
        direction  = ((double)random()/RAND_MAX) * (2.0 * M_PI);
        p->x       = distance * cos(direction);
        p->y       = distance * sin(direction);

        speed      = random_triangular(-sim->initial_avg_speed/0.292, sim->initial_avg_speed/0.292);
        direction  = ((double)random()/RAND_MAX) * (2.0 * M_PI);
        p->x_speed = speed * cos(direction);
        p->y_speed = speed * sin(direction);
    }

    // init control variables
    state = STATE_STOP;
    display_width = 100000; // XXX tbd

    // create worker threads
#ifndef ANDROID
    max_thread = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    if (max_thread == 0) {
        max_thread = 1;
    } else if (max_thread > MAX_THREAD) {
        max_thread = MAX_THREAD;
    }
#else
    max_thread = 1;
#endif
    INFO("max_thread=%dd\n", max_thread);
    pthread_barrier_init(&barrier,NULL,max_thread);
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
    pthread_barrier_destroy(&barrier);
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
    sdl_event_t * event;
    int32_t       simpane_width, color;
    char          str[100];
    int32_t       next_display = -1;

    static SDL_Texture * circle_texture[MAX_COLOR];       
    
    // init circles
    for (color = 0; color < MAX_COLOR; color++) {
        if (circle_texture[color] == NULL) {
            circle_texture[color] = sdl_create_filled_circle_texture(1, sdl_pixel_rgba[color]);
        }
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

#if 0
        // int32_t i, win_x, win_y;
        for (i = 0; i < sim->max_particle; i++) {
            particle_t * p = &sim->particle[i];

            win_x = 0; // XXX (p->x + sim->sim_width/2) * simpane_width / sim->sim_width;
            win_y = 0; // XXX (p->y + sim->sim_width/2) * simpane_width / sim->sim_width;
            sdl_render_circle(win_x, win_y, circle_texture[SPEED_TO_COLOR(p->speed)]);
        }
#endif

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

        //
        // display status lines
        //

        sprintf(str, "%s %0.1lE ", STATE_STR(state), sim->curr_year);
        sdl_render_text_font0(&ctlpane,  6, 0, str, SDL_EVENT_NONE);

        // xxx use different units
        sdl_render_text_font0(&ctlpane, 12, 0, "PARAMS ...", SDL_EVENT_NONE);
        sprintf(str, "N_PART = %ld", sim->max_particle);    
        sdl_render_text_font0(&ctlpane, 13, 0, str, SDL_EVENT_NONE);
        sprintf(str, "RADIUS = %0.1lE", sim->initial_radius);    
        sdl_render_text_font0(&ctlpane, 14, 0, str, SDL_EVENT_NONE);
        sprintf(str, "SPEED  = %0.1lE", sim->initial_avg_speed);    
        sdl_render_text_font0(&ctlpane, 15, 0, str, SDL_EVENT_NONE);
        sprintf(str, "DOUBLE = %0.1lE", sim->radius_double_years);    
        sdl_render_text_font0(&ctlpane, 16, 0, str, SDL_EVENT_NONE);

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
            // XXX int32_t max_particle = sim->max_particle;
            // int64_t sim_width    = sim->sim_width;
            state = STATE_STOP;
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
    int64_t max_particle        = sim->max_particle;
    double  initial_radius      = sim->initial_radius;
    double  initial_avg_speed   = sim->initial_avg_speed;
    double  radius_double_years = sim->radius_double_years;

    char cur_s1[100], cur_s2[100], cur_s3[100], cur_s4[100];
    char ret_s1[100], ret_s2[100], ret_s3[100], ret_s4[100];

    // get new value strings for the params
    sprintf(cur_s1, "%ld", max_particle);
    sprintf(cur_s2, "%0.1lE", initial_radius);
    sprintf(cur_s3, "%0.1lE", initial_avg_speed);
    sprintf(cur_s4, "%0.1lE", radius_double_years);
    sdl_display_get_string(4,
                           "N_PART", cur_s1, ret_s1,
                           "RADIUS", cur_s2, ret_s2,
                           "SPEED",  cur_s3, ret_s3,
                           "DOUBLE", cur_s4, ret_s4);

    // scan returned strings
    sscanf(ret_s1, "%ld", &max_particle);
    sscanf(ret_s2, "%lE", &initial_radius);
    sscanf(ret_s3, "%lE", &initial_avg_speed);
    sscanf(ret_s4, "%lE", &radius_double_years);

    // xxx check ranges

    // re-init with new params
    sim_universe_terminate();
    sim_universe_init(max_particle, initial_radius, initial_avg_speed, radius_double_years);

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
    int32_t         id = (long)cx;

    INFO("thread starting, id=%d\n", id);
    pthread_barrier_wait(&barrier);

    pthread_barrier_wait(&barrier);
    INFO("thread terminating, id=%d\n", id);
    return NULL;
}

#if 0 // xxx todo
    #define BATCH_SIZE 100 
    #define PB(n)      (pb_buff + (n) * sim_size)


    int32_t         pb_end, pb_beg, pb_idx, pb_max=0;
    int64_t         pb_tail;
    void          * pb_buff = NULL;

    static int64_t  batch_idx;
    static int32_t  curr_state;
    static int32_t  last_state;
    static int64_t  sim_size;
    static int64_t  cont_width;
    static int32_t  cont_speed;

    INFO("thread starting, id=%d\n", id);
    if (id == 0) {
        batch_idx = 0;
        curr_state = STATE_STOP;
        last_state = STATE_STOP;
        sim_size   = SIM_SIZE(sim->max_particle);
        cont_width = sim->sim_width;
        cont_speed = 0;

        pb_max = 10000;
        while (true) {
            pb_buff = calloc(pb_max, sim_size);
            if (pb_buff != NULL) {
                INFO("playback buff_size %d MB, pb_max=%d\n", (int32_t)((pb_max * sim_size) / MB), pb_max);
                break;
            }
            pb_max -= 100;
            if (pb_max <= 0) {
                FATAL("failed allocate playback buffer\n");
            }
        }
        pb_end = pb_beg = pb_idx = 0;
        memcpy(PB(0), sim, sim_size);
        pb_tail = 1;
    }
    pthread_barrier_wait(&barrier);

    while (true) {
        // thread id 0 performs control functions
        if (id == 0) {
            // determine current and last state
            last_state = curr_state;
            curr_state = state;

            // if state has changed, and the current state is not playback then 
            // reset sim to the last copy saved in the playback buffer array
            if ((curr_state != last_state) &&
                (curr_state == STATE_RUN || 
                 curr_state == STATE_STOP ||
                 curr_state == STATE_LOW_ENTROPY_STOP))
            {
                memcpy(sim, PB((pb_tail-1)%pb_max), sim_size);
            }

            // determine cont_speed and cont_width
            if (state == STATE_RUN) {
                cont_speed = cont_shrink_restore_speed;
                cont_width = sim->cont_width + 2 * cont_speed * (PARTICLE_DIAMETER / SPEED_AVG / 10);
                if (cont_width < (int64_t)sim->sim_width*75/100) {
                    cont_width = (int64_t)sim->sim_width*75/100;
                    cont_speed = 0;
                    cont_shrink_restore_speed = 0;
                }
                if (cont_width > sim->sim_width) {
                    cont_width = sim->sim_width;
                    cont_speed = 0;
                    cont_shrink_restore_speed = 0;
                }
                sim->cont_width = cont_width;
            }
        }

        // all threads rendezvoud here
        pthread_barrier_wait(&barrier);

        // process based on curr_state
        if (curr_state == STATE_RUN) {
            int32_t batch_start, batch_end, num_in_batch, i;
            static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

            while (true) {
                // get a batch of particle to process
                pthread_mutex_lock(&mutex);
                if (batch_idx / sim->max_particle > sim->state_num) {
                    pthread_mutex_unlock(&mutex);
                    break;
                }
                batch_start = batch_idx % sim->max_particle;
                num_in_batch = batch_start + BATCH_SIZE > sim->max_particle ? sim->max_particle - batch_start : BATCH_SIZE;
                batch_end = batch_start + num_in_batch;
                batch_idx += num_in_batch;
                pthread_mutex_unlock(&mutex);

                // process the batch
                for (i = batch_start; i < batch_end; i++) {
                    particle_t * p = &sim->particle[i];
                    int64_t new_x, new_y; 

                    // determine particles new x,y position
                    new_x = p->x + p->x_speed * (PARTICLE_DIAMETER / SPEED_AVG / 10);
                    new_y = p->y + p->y_speed * (PARTICLE_DIAMETER / SPEED_AVG / 10);

                    // if the new position is within the universe then
                    // assign the new position to the particle and continue
                    if (new_x >= -cont_width/2 &&
                        new_x <= cont_width/2 &&
                        new_y >= -cont_width/2 &&
                        new_y <= cont_width/2)
                    {
                        p->x = new_x;
                        p->y = new_y;
                        continue;
                    }

                    // the following code handles particle collision with the
                    // sides of the universe; note that if the particle goes off
                    // a corner then this code handles that by processing collision 
                    // off of 2 sides
                    double q, rand_dir; 
                    double new_speed=0, new_x_speed=0, new_y_speed=0;

                    rand_dir = (double)random_uniform(500,2000) / 1000;  // range 0.5 - 2.0
                    if (fabs(p->x_speed)  < 1) p->x_speed = 1;
                    if (fabs(p->y_speed)  < 1) p->y_speed = 1;

                    if (new_x < -cont_width/2) {
                        new_x = -cont_width/2;
                        new_speed = p->speed - cont_speed;
                        new_speed += ((SPEED_AVG - new_speed) / 50.) + random_triangular(-100,100)/100.;
                        q = (double)p->y_speed / abs(p->x_speed) * rand_dir;
                        new_x_speed = sqrt( (new_speed * new_speed) / (1. + q * q) );
                        new_y_speed = q * new_x_speed;
                    }

                    if (new_x > cont_width/2) {
                        new_x = cont_width/2;
                        new_speed = p->speed - cont_speed;
                        new_speed += ((SPEED_AVG - new_speed) / 50.) + random_triangular(-100,100)/100.;
                        q = (double)p->y_speed / -abs(p->x_speed) * rand_dir;
                        new_x_speed = -sqrt( (new_speed * new_speed) / (1. + q * q) );
                        new_y_speed = q * new_x_speed;
                    }

                    if (new_y < -cont_width/2) {
                        new_y = -cont_width/2;
                        new_speed = p->speed - cont_speed;
                        new_speed += ((SPEED_AVG - new_speed) / 50.) + random_triangular(-100,100)/100.;
                        q = (double)p->x_speed / abs(p->y_speed) * rand_dir;
                        new_y_speed = sqrt( (new_speed * new_speed) / (1. + q * q) );
                        new_x_speed = q * new_y_speed;
                    }

                    if (new_y > cont_width/2) {
                        new_y = cont_width/2;
                        new_speed = p->speed - cont_speed;
                        new_speed += ((SPEED_AVG - new_speed) / 50.) + random_triangular(-100,100)/100.;
                        q = (double)p->x_speed / -abs(p->y_speed) * rand_dir;
                        new_y_speed = -sqrt( (new_speed * new_speed) / (1. + q * q) );
                        new_x_speed = q * new_y_speed;
                    }

                    p->x = new_x;
                    p->y = new_y;
                    p->x_speed = new_x_speed;
                    p->y_speed = new_y_speed;
                    p->speed = new_speed;
                }
            }

            // all threads rendezvoud here
            pthread_barrier_wait(&barrier);

            // all particles have been updated, thread 0 does post processing
            if (id == 0) {
                // increment the state_num
                sim->state_num++;

                // check for auto stop, this occurs when initial condition is closely reproduced
                if (sim_universe_should_auto_stop()) {
                    state = STATE_LOW_ENTROPY_STOP;
                }

                // make playback copy
                memcpy(PB(pb_tail%pb_max), sim, sim_size);
                pb_tail++;

                // sleep to pace the simulation
                if (RUN_SPEED_SLEEP_TIME > 0) {
                    usleep(RUN_SPEED_SLEEP_TIME);
                }
            }

        } else if (curr_state == STATE_PLAYBACK_REV) {
            if (id == 0) {
                if (last_state != STATE_PLAYBACK_REV &&
                    last_state != STATE_PLAYBACK_FWD &&
                    last_state != STATE_PLAYBACK_STOP) 
                {
                    pb_end = (pb_tail - 1) % pb_max;
                    pb_beg = (pb_tail <= pb_max ? 0 : (pb_tail + 1) % pb_max);
                    pb_idx = pb_end;
                }

                memcpy(sim, PB(pb_idx), sim_size);

                if (pb_idx == pb_beg) {
                    state = STATE_PLAYBACK_STOP;
                } else {
                    pb_idx = pb_idx - 1;
                    if (pb_idx < 0) {
                        pb_idx += pb_max;
                    }
                    if (RUN_SPEED_SLEEP_TIME > 0) {
                        usleep(RUN_SPEED_SLEEP_TIME);
                    }
                }
            }

        } else if (curr_state == STATE_PLAYBACK_FWD) {
            if (id == 0) {
                if (last_state != STATE_PLAYBACK_REV &&
                    last_state != STATE_PLAYBACK_FWD &&
                    last_state != STATE_PLAYBACK_STOP) 
                {
                    pb_end = (pb_tail - 1) % pb_max;
                    pb_beg = (pb_tail <= pb_max ? 0 : (pb_tail + 1) % pb_max);
                    pb_idx = pb_end;
                }

                memcpy(sim, PB(pb_idx), sim_size);

                if (pb_idx == pb_end) {
                    state = STATE_PLAYBACK_STOP;
                } else  {
                    pb_idx = pb_idx + 1;
                    if (pb_idx >= pb_max) {
                        pb_idx -= pb_max;
                    }
                    if (RUN_SPEED_SLEEP_TIME > 0) {
                        usleep(RUN_SPEED_SLEEP_TIME);
                    }
                }
            }

        } else if (curr_state == STATE_TERMINATE_THREADS) {
            if (id == 0) {
                free(pb_buff);
                pb_buff = NULL;
            }
            break;

        } else {
            // the STOP states
            usleep(50000);
        }
    }
#endif


