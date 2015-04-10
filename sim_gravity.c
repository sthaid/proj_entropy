// XXX how to use color
// XXX fast circles

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#include "util.h"

//
// defines
//

#define MB 0x100000

#define MAX_THREAD 16

#define SDL_EVENT_STOP           (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN            (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_SLOW           (SDL_EVENT_USER_START + 2)
#define SDL_EVENT_FAST           (SDL_EVENT_USER_START + 3)
#define SDL_EVENT_PLAYBACK_REV   (SDL_EVENT_USER_START + 6)
#define SDL_EVENT_PLAYBACK_FWD   (SDL_EVENT_USER_START + 7)
#define SDL_EVENT_RESET          (SDL_EVENT_USER_START + 9)
#define SDL_EVENT_RESET_PARAMS   (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_BACK           (SDL_EVENT_USER_START + 11)
// xxx pan and zoom
// xxx configure new sim
// xxx configure from saved sim

#define PURPLE 380  // XXX move to util.h
#define GREEN  540
#define RED    700
#if 0 // XXX move to util.h and generalize
#define WAVELENGTH(speed) \
        (PURPLE + ((speed) - (SPEED_AVG-SPEED_SPREAD/2)) * (RED - PURPLE) / SPEED_SPREAD)
#endif

#define STATE_STOP               0
#define STATE_RUN                1
#define STATE_PLAYBACK_REV       3
#define STATE_PLAYBACK_FWD       4
#define STATE_PLAYBACK_STOP      5
#define STATE_TERMINATE_THREADS  9

#define STATE_STR(s) \
    ((s) == STATE_STOP              ? "STOP"              : \
     (s) == STATE_RUN               ? "RUN"               : \
     (s) == STATE_PLAYBACK_REV      ? "PLAYBACK_REV"      : \
     (s) == STATE_PLAYBACK_FWD      ? "PLAYBACK_FWD"      : \
     (s) == STATE_PLAYBACK_STOP     ? "PLAYBACK_STOP"     : \
     (s) == STATE_TERMINATE_THREADS ? "TERMINATE_THREADS"   \
                                    : "????")

#define MIN_RUN_SPEED     1
#define MAX_RUN_SPEED     9
#define DEFAULT_RUN_SPEED 5
#define RUN_SPEED_SLEEP_TIME (100 * ((1 << (MAX_RUN_SPEED-run_speed)) - 1))

//
// typedefs
//

typedef struct {
    long double x;  // XXX check size and precision, maybe use int128 if available on android
    long double y;
    long double x_speed;
    long double y_speed;
} object_t; 

typedef struct {
    int64_t state_num;
    int64_t sim_size; 
    int32_t max_object;
    object_t object[0];
} sim_t;

typedef struct {
    int32_t id;
    int32_t first_obj;
    int32_t last_obj;
} thread_cx_t;

//
// variables
//

static sim_t           * sim;

static int32_t           state; 
static int32_t           run_speed;
static double            sim_width;

static int32_t           max_thread;
static pthread_t         thread_id[MAX_THREAD];
static pthread_barrier_t barrier;

// 
// prototypes
//

int32_t sim_gravity_init(void);
void sim_gravity_terminate(void);
bool sim_gravity_display(void);
void * sim_gravity_thread(void * cx);

// -----------------  MAIN  -----------------------------------------------------

void sim_gravity(void) 
{
    INFO("sizeof double = %d\n", (int)sizeof(double));
    INFO("sizeof long double = %d\n", (int)sizeof(long double));
    //INFO("sizeof long long double = %d\n", (int)sizeof(quad double));

    if (sim_gravity_init() != 0) {
        return;
    }

    while (true) {
        bool done = sim_gravity_display();
        if (done) {
            break;
        }
    }

    sim_gravity_terminate();
}

// -----------------  INIT & TERMINATE  -----------------------------------------

int32_t sim_gravity_init(void) 
{
    #define SIM_SIZE(n) (sizeof(sim_t) + (n) * sizeof(object_t))

    int32_t i, num_proc;
    int64_t sim_size;

    // print info msg
    INFO("initialize start\n");

    int32_t max_object = 2;  //XXX
 
    // allocate sim
    free(sim);
    sim_size = SIM_SIZE(max_object);
    sim = calloc(1, sim_size);
    if (sim == NULL) {
        return -1;
    }

    // set simulation state
    sim->state_num  = 0;
    sim->sim_size   = sim_size;
    sim->max_object = max_object;
    // XXX print the sizeof doubles
    // XXX is 0 init okay for obj

    // init control variables
    state = STATE_STOP;
    run_speed = DEFAULT_RUN_SPEED;
    sim_width = 100000000;

    // create worker threads
    // XXX do this based on number of objects, pass
    //   each thread a context which is the first and count of its objects, and whether barrier is needed
    num_proc = sysconf(_SC_NPROCESSORS_ONLN);
    max_thread = 1;
    INFO("max_thread=%d num_proc=%d\n", max_thread, num_proc);
    pthread_barrier_init(&barrier,NULL,max_thread);
    for (i = 0; i < max_thread; i++) {
        thread_cx_t * cx = malloc(sizeof(thread_cx_t));
        cx->id        = i;
        cx->first_obj = 0;
        cx->last_obj  = 1;
        pthread_create(&thread_id[i], NULL, sim_gravity_thread, cx);
    }

    // success
    INFO("initialize complete\n");
    return 0;
}

void sim_gravity_terminate(void)
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

    // free allocations 
    pthread_barrier_destroy(&barrier);
    free(sim);
    sim = NULL;

    // done
    INFO("terminate complete\n");
}

// -----------------  DISPLAY  --------------------------------------------------

bool sim_gravity_display(void)
{
    SDL_Rect       ctlpane;
    SDL_Rect       rect;
    int32_t        simpane_width, count, event, i;
    int32_t        win_x, win_y, win_r;
    uint8_t        r, g, b;
    char           str[100];
    bool           done = false;

#if 0
    int32_t        i, event, simpane_width, cont_width, count;
    double         sum_speed, temperature;
#endif
    
    // init
    if (sdl_win_width > sdl_win_height) {
        simpane_width = sdl_win_height;
        SDL_INIT_PANE(ctlpane,
                      simpane_width, 0,             // x, y
                      sdl_win_width-simpane_width, sdl_win_height);     // w, h
    } else {
        simpane_width = sdl_win_width;
        SDL_INIT_PANE(ctlpane, 0, 0, 0, 0);
    }
    sdl_event_init();

    // if in a stopped state then delay for 50 ms, to reduce cpu usage
    count = 0;
    while ((state == STATE_STOP ||
            state == STATE_PLAYBACK_STOP) &&
           (count < 100))
    {
        count++;
        usleep(1000);
    }

    // clear window
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);

    // draw object



    //win_r = simpane_width / (sim->sim_width/OBJECT_DIAMETER) / 2;
    //if (win_r < 2) {
        //win_r = 2;
    //}
    // XXX may want to display a tail
    win_r = 5;
    wavelength_to_rgb(GREEN, &r, &g, &b);
    SDL_SetRenderDrawColor(sdl_renderer, r, g, b, SDL_ALPHA_OPAQUE);

    for (i = 0; i < sim->max_object; i++) {
        SDL_Rect rect;
        object_t * p = &sim->object[i];

        win_x = (p->x + sim_width/2) * (simpane_width / sim_width);
        win_y = (p->y + sim_width/2) * (simpane_width / sim_width);

        rect.x = win_x - win_r;
        rect.y = win_y - win_r;
        rect.w = 2 * win_r;
        rect.h = 2 * win_r;

        SDL_RenderFillRect(sdl_renderer, &rect);
    }

    // clear ctlpane
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(sdl_renderer, &ctlpane);

    // display controls
    sdl_render_text_font0(&ctlpane,  0, 3,  "GRAVITY",         SDL_EVENT_NONE);
    sdl_render_text_font0(&ctlpane,  2, 0,  "RUN",             SDL_EVENT_RUN);
    sdl_render_text_font0(&ctlpane,  2, 9,  "STOP",            SDL_EVENT_STOP);
    sdl_render_text_font0(&ctlpane,  4, 0,  "SLOW",            SDL_EVENT_SLOW);
    sdl_render_text_font0(&ctlpane,  4, 9,  "FAST",            SDL_EVENT_FAST);
    sdl_render_text_font0(&ctlpane,  6, 0,  "PB_REV",          SDL_EVENT_PLAYBACK_REV);
    sdl_render_text_font0(&ctlpane,  6, 9,  "PB_FWD",          SDL_EVENT_PLAYBACK_FWD);
    sdl_render_text_font0(&ctlpane,  8, 0,  "RESET",           SDL_EVENT_RESET);
    sdl_render_text_font0(&ctlpane,  8, 9,  "RESET_PARAM",     SDL_EVENT_RESET_PARAMS);
    sdl_render_text_font0(&ctlpane, 18,15,  "BACK",            SDL_EVENT_BACK);

    // display status lines
    sprintf(str, "%s %"PRId64" ", STATE_STR(state), sim->state_num);
    sdl_render_text_font0(&ctlpane, 12, 0, str, SDL_EVENT_NONE);
    sprintf(str, "RUN_SPEED   = %d", run_speed);    
    sdl_render_text_font0(&ctlpane, 15, 0, str, SDL_EVENT_NONE);

    // draw border
    wavelength_to_rgb(GREEN, &r, &g, &b);
    rect.x = 0;
    rect.y = 0;
    rect.w = simpane_width;
    rect.h = simpane_width;
    sdl_render_rect(&rect, 3, r, g, b, SDL_ALPHA_OPAQUE);

    // present the display
    SDL_RenderPresent(sdl_renderer);

    // handle events
    event = sdl_poll_event();
    switch (event) {
    case SDL_EVENT_RUN:
        state = STATE_RUN;
        break;
    case SDL_EVENT_STOP:
        if (state == STATE_RUN) {
            state = STATE_STOP;
        } else if (state == STATE_PLAYBACK_REV || state == STATE_PLAYBACK_FWD) {
            state = STATE_PLAYBACK_STOP;
        }
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
    case SDL_EVENT_PLAYBACK_REV:
        state = STATE_PLAYBACK_REV;
        break;
    case SDL_EVENT_PLAYBACK_FWD:
        state = STATE_PLAYBACK_FWD;
        break;
    case SDL_EVENT_RESET:
    case SDL_EVENT_RESET_PARAMS: {
        sim_gravity_terminate();  //XXX parms tbd
        sim_gravity_init();
        break; }
    case SDL_EVENT_BACK: 
    case SDL_EVENT_QUIT:
        done = true;
        break;
    default:
        break;
    }

    // return done flag
    return done;
}

// -----------------  THREAD  ---------------------------------------------------

void * sim_gravity_thread(void * cx_arg)
{
    #define PB(n)      (pb_buff + (n) * sim->sim_size)

    thread_cx_t     cx = *(thread_cx_t*)cx_arg;
    int32_t         pb_end, pb_beg, pb_idx, pb_max=0;
    int64_t         pb_tail;
    void          * pb_buff = NULL;

    static int32_t  curr_state;
    static int32_t  last_state;

    // a copy of cx_arg has been made, so free cx_arg
    free(cx_arg);

    // all threads print starting message
    INFO("thread starting, id=%d first_obj=%d last_obj=%d\n", cx.id, cx.first_obj, cx.last_obj);

    // thread 0 initializes
    if (cx.id == 0) {
        curr_state = STATE_STOP;
        last_state = STATE_STOP;

        pb_max = 10000;
        while (true) {
            pb_buff = calloc(pb_max, sim->sim_size);
            if (pb_buff != NULL) {
                INFO("playback buff_size %d MB, pb_max=%d\n", (int32_t)((pb_max * sim->sim_size) / MB), pb_max);
                break;
            }
            pb_max -= 100;
            if (pb_max <= 0) {
                FATAL("failed allocate playback buffer\n");
            }
        }
        pb_end = pb_beg = pb_idx = 0;
        memcpy(PB(0), sim, sim->sim_size);
        pb_tail = 1;
    }
    pthread_barrier_wait(&barrier);

    while (true) {
        // thread id 0 performs control functions
        if (cx.id == 0) {
            // determine current and last state
            last_state = curr_state;
            curr_state = state;

            // if state has changed, and the current state is not playback then 
            // reset sim to the last copy saved in the playback buffer array
            if ((curr_state != last_state) &&
                (curr_state == STATE_RUN || 
                 curr_state == STATE_STOP))
            {
                memcpy(sim, PB((pb_tail-1)%pb_max), sim->sim_size);
            }
        }

        // all threads rendezvoud here
        pthread_barrier_wait(&barrier);  // XXX don't if max_thread==1

        // process based on curr_state
        if (curr_state == STATE_RUN) {

#if 0
        Mself * Mi
F = G * ----------
           Ri^2
                            DeltaXi                 Mi * DeltaXi
Fx = F * cos(theta) = F * ----------- = G * Mself * ------------
                              Ri                       Ri^3

                      Mi * DeltaXi
Fx = G * Mself * SUM(-------------)
                         Ri^3

                           1     Fx
DeltaX = Xspeed * DeltaT + - * ------ * DeltaT^2
                           2   Mself

                           1                       Mi * DeltaXi
DeltaX = Xspeed * DeltaT + - * DeltaT^2 * G * SUM(-------------)
                           2                          Ri^3
#endif

            // XXX tbd

            // all threads rendezvoud here
            pthread_barrier_wait(&barrier);

            // all particles have been updated, thread 0 does post processing
            if (cx.id == 0) {
                // increment the state_num
                sim->state_num++;

                // make playback copy
                memcpy(PB(pb_tail%pb_max), sim, sim->sim_size);
                pb_tail++;

                // sleep to pace the simulation
                if (RUN_SPEED_SLEEP_TIME > 0) {
                    usleep(RUN_SPEED_SLEEP_TIME);
                }
            }


        } else if (curr_state == STATE_PLAYBACK_REV) {
            if (cx.id == 0) {
                if (last_state != STATE_PLAYBACK_REV &&
                    last_state != STATE_PLAYBACK_FWD &&
                    last_state != STATE_PLAYBACK_STOP) 
                {
                    pb_end = (pb_tail - 1) % pb_max;
                    pb_beg = (pb_tail <= pb_max ? 0 : (pb_tail + 1) % pb_max);
                    pb_idx = pb_end;
                }

                memcpy(sim, PB(pb_idx), sim->sim_size);

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
            if (cx.id == 0) {
                if (last_state != STATE_PLAYBACK_REV &&
                    last_state != STATE_PLAYBACK_FWD &&
                    last_state != STATE_PLAYBACK_STOP) 
                {
                    pb_end = (pb_tail - 1) % pb_max;
                    pb_beg = (pb_tail <= pb_max ? 0 : (pb_tail + 1) % pb_max);
                    pb_idx = pb_end;
                }

                memcpy(sim, PB(pb_idx), sim->sim_size);

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
            if (cx.id == 0) {
                free(pb_buff);
                pb_buff = NULL;
            }
            break;

        } else {
            // the STOP states
            usleep(50000);
        }
    }

    pthread_barrier_wait(&barrier);
    INFO("thread terminating, id=%d\n", cx.id);
    return NULL;
}
