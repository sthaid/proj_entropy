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

#define DEFAULT_MAX_PARTICLE        (1000000)      // 1 million
#define DEFAULT_INITIAL_RADIUS      (0.5E9 * 1E6)  // 0.5  billion LY
#define DEFAULT_INITIAL_AVG_SPEED   (0.10 * 1E6)   // .10 C
#define DEFAULT_RADIUS_DOUBLE_INTVL (2E9)          // 2 billion years
#define DEFAULT_DELTA_TIME          (1)            // 1 year
#define DEFAULT_DISPLAY_WIDTH       (10E9 * 1E6)   // 10 billion LY

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
    int64_t  x;           // 1E-6 LY
    int64_t  y;        
    int64_t  x_speed;     // 1E-6 LY / Year
    int64_t  y_speed;
} particle_t;

typedef struct {
    // params
    int64_t  max_particle;
    int64_t  initial_radius;       // 1E-6 LY
    int64_t  initial_avg_speed;    // 1E-6 LY / Year
    int64_t  radius_double_intvl;  // Years

    // simulation
    int64_t   time;                // Years
    int64_t   radius;              // 1E-6 LY
    particle_t particle[0];
} sim_t;

//
// variables
//

static sim_t           * sim;

static int32_t           state; 
static int64_t           delta_time;     // Years
static int64_t           display_width;  // 1E-6 LY

static int32_t           max_thread;
static pthread_t         thread_id[MAX_THREAD];
static pthread_barrier_t barrier;

// 
// prototypes
//

int32_t sim_universe_init(int64_t max_particle, int64_t initial_radius,
                          int64_t initial_avg_speed, int64_t radius_double_intvl);
void sim_universe_terminate(void);
void sim_universe_display(void);
int32_t sim_universe_display_simulation(int32_t curr_display, int32_t last_display);
int32_t sim_universe_display_select_params(int32_t curr_display, int32_t last_display);
int32_t sim_universe_display_help(int32_t curr_display, int32_t last_display);
void * sim_universe_thread(void * cx);

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
    int64_t max_particle,
    int64_t initial_radius,
    int64_t initial_avg_speed,
    int64_t radius_double_intvl)
{
    #define SIM_SIZE(n) (sizeof(sim_t) + (n) * sizeof(particle_t))

    int32_t i;

    // print info msg
    INFO("initialize start: max_particle=%ld initial_radius=%ld"
         " initial_avg_speed=%ld radius_double_intvl=%ld\n",
         max_particle, initial_radius, initial_avg_speed, radius_double_intvl);

    // allocate sim
    free(sim);
    sim = calloc(1, SIM_SIZE(max_particle));
    if (sim == NULL) {
        // xxx fake in a dummy sim that is malloced but with small max_particles
        return -1;
    }

    // initialize sim
    // xxx maybe the threads can do this init quicker
    sim->max_particle        = max_particle;
    sim->initial_radius      = initial_radius;
    sim->initial_avg_speed   = initial_avg_speed;   // xxx limit on this  0.14
    sim->radius_double_intvl = radius_double_intvl;
    sim->time                = 0;
    sim->radius              = initial_radius;
    for (i = 0; i < max_particle; i++) {
        particle_t * p = &sim->particle[i];
        double  direction, distance, speed;

        distance   = sqrt((double)random()/RAND_MAX) * (initial_radius);
        direction  = ((double)random()/RAND_MAX) * (2.0 * M_PI);
        p->x       = distance * cos(direction);  // xxx table lookup for sin/cos
        p->y       = distance * sin(direction);

        speed      = random_triangular(-3*initial_avg_speed, 3*initial_avg_speed);
        direction  = ((double)random()/RAND_MAX) * (2.0 * M_PI);
        p->x_speed = speed * cos(direction);
        p->y_speed = speed * sin(direction);
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
    int32_t       simpane_width, color, i;
    char          str[100];
    int32_t       next_display = -1;

    static SDL_Texture * circle_texture[MAX_COLOR];       
    
    // init circles
    // xxx maybe use points or rectangles for faster drawing
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

        static int32_t loop_count;
        PRINTF("DRAW PARTICLES COUNT %d\n", loop_count++);
#if 0
        for (i = 0; i < sim->max_particle; i++) {
            particle_t * p = &sim->particle[i];
            int32_t      win_x, win_y;

            win_x = (p->x + display_width/2) * simpane_width / display_width;
            win_y = (p->y + display_width/2) * simpane_width / display_width;
            sdl_render_circle(win_x, win_y, circle_texture[WHITE]);
        }
#else
        static SDL_Point points[1000000];
        int32_t count;
        count = 0;
        for (i = 0; i < sim->max_particle; i++) {
            particle_t * p = &sim->particle[i];

            points[count].x = (p->x + display_width/2) * simpane_width / display_width;
            points[count].y = (p->y + display_width/2) * simpane_width / display_width;
            count++;
        }
        SDL_SetRenderDrawColor(sdl_renderer, 255,255,255, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawPoints(sdl_renderer, points, count);
#endif
        PRINTF(" ... done\n");

        //
        // clear ctlpane
        //

        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(sdl_renderer, &ctlpane);

        //
        // display controls
        // xxx add DELTA_TIME controls
        // xxx add DISPLAY_WIDTH controls
        // xxx add pan control
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

        // - state & time
        sprintf(str, "%-4s %0.9lf BYR", STATE_STR(state), (double)sim->time/1000000000);
        sdl_render_text_font0(&ctlpane,  6, 0, str, SDL_EVENT_NONE);

        // - delta_time
        sprintf(str, "DELTA %ld YR", delta_time);
        sdl_render_text_font0(&ctlpane,  7, 0, str, SDL_EVENT_NONE);

        // - params
        sdl_render_text_font0(&ctlpane, 12, 0, "PARAMS ...", SDL_EVENT_NONE);
        sprintf(str, "N_PART = %ld", sim->max_particle);    
        sdl_render_text_font0(&ctlpane, 13, 0, str, SDL_EVENT_NONE);
        sprintf(str, "RADIUS = %0.3lf BLY", (double)sim->initial_radius/(1E9*1E6));
        sdl_render_text_font0(&ctlpane, 14, 0, str, SDL_EVENT_NONE);
        sprintf(str, "SPEED  = %0.3lf C", (double)sim->initial_avg_speed/1E6);
        sdl_render_text_font0(&ctlpane, 15, 0, str, SDL_EVENT_NONE);
        sprintf(str, "DOUBLE = %0.3lf BYR", (double)sim->radius_double_intvl/1E9);
        sdl_render_text_font0(&ctlpane, 16, 0, str, SDL_EVENT_NONE);

        // - window width
        sprintf(str, "WIDTH %0.3lf BLY", (double)display_width/(1E9*1E6));
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
            // xxx int32_t max_particle = sim->max_particle;
            // int64_t sim_width    = sim->sim_width;
            state = STATE_STOP;
// xxx preserve if args are 0
            // sim_universe_terminate();
            // sim_universe_init(max_particle, sim_width);
            break; }
// xxx DT
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
    int64_t initial_radius      = sim->initial_radius;
    int64_t initial_avg_speed   = sim->initial_avg_speed;
    int64_t radius_double_intvl = sim->radius_double_intvl;
    double  val;

    char cur_s1[100], cur_s2[100], cur_s3[100], cur_s4[100];
    char ret_s1[100], ret_s2[100], ret_s3[100], ret_s4[100];

    // get new value strings for the params
    sprintf(cur_s1, "%ld",    max_particle);
    sprintf(cur_s2, "%0.3lf", (double)initial_radius/(1E9*1E6));
    sprintf(cur_s3, "%0.3lf", (double)initial_avg_speed/1E6);
    sprintf(cur_s4, "%0.3lf", (double)radius_double_intvl/1E9);
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
        initial_radius = val * (1E9*1E6);
    }
    if (sscanf(ret_s3, "%lf", &val) == 1) {
        initial_avg_speed = val * 1E6;
    }
    if (sscanf(ret_s4, "%lf", &val) == 1) {
        radius_double_intvl = val * 1E9;
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

    int32_t         id = (long)cx;

    static int64_t  batch_idx;
    static int64_t  batch_count;
    static int32_t  curr_state;

    INFO("thread starting, id=%d\n", id);
    if (id == 0) {
        batch_idx  = 0;
        batch_count = 0;
        curr_state = STATE_STOP;
    }
    pthread_barrier_wait(&barrier);

    while (true) {
        // thread 0 sets the curr_state global var
        if (id == 0) {
            curr_state = state;
        }

        // rendezvous
        pthread_barrier_wait(&barrier);

        // process based on curr_state
        if (curr_state == STATE_RUN) {
            int32_t batch_start, batch_end, num_in_batch, i;
            static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

                // process the batch
                for (i = batch_start; i < batch_end; i++) {
                    //particle_t * p = &sim->particle[i];

                    // XXX equations

                }
            }

            // rendezvous
            pthread_barrier_wait(&barrier);

            // all particles have been updated, thread 0 does post processing
            if (id == 0) {
                batch_count++;
            }

        } else if (curr_state == STATE_TERMINATE_THREADS) {
            break;

        } else {  // STATE_STOP
            usleep(50000);
        }
    }

    pthread_barrier_wait(&barrier);
    INFO("thread terminating, id=%d\n", id);
    return NULL;
}

