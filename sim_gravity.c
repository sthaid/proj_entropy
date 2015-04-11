// XXX fast circles
// xxx how to use color
// xxx may want to display a tail

//            DOUBLE      LONG DOUBLE
// LINUX        8            10 
// ANDROID      8             8
//
// Distance to Pluto = 
//   13 light hours
//   1.4E13 meters
//   1.4E16 mm
//
// 2^63 = 9E18
//
// Range of Floating point types:
//   float        4 byte    1.2E-38   to 3.4E+38      6 decimal places
//   double       8 byte    2.3E-308  to 1.7E+308    15 decimal places
//   long double 10 byte    3.4E-4932 to 1.1E+4932   19 decimal places

// -----------------  BEGIN  --------------------------------------------------------------------------

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

#define PURPLE 380  // xxx move to util.h
#define BLUE   440
#define GREEN  540
#define RED    700
#if 0 // xxx move to util.h and generalize
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
#define DEFAULT_RUN_SPEED 9
#define RUN_SPEED_SLEEP_TIME (100 * ((1 << (MAX_RUN_SPEED-run_speed)) - 1))

#define TWO_PI (2.0*M_PI)

#define DT             1.0                  // second

#define G              6.673E-11            // mks units  
#define AU             149597870700.0       // meters XXX cvt to scientific

#define M_SUN          1.989E30             // kg

#define M_EARTH        5.972E24             // kg
#define R_EARTH        AU                   // orbit radius in meters
#define T_EARTH        (365.256*86400)      // orbit time in seconds

#define M_MOON         7.347E22             // kg
#define R_MOON         3.844E8              // orbit radius in meters
#define T_MOON         (27.3217*86400)      // orbit time in seconds

#define M_VENUS        4.867E24             // kg
#define R_VENUS        1.0821E11            // orbit radius in meters
#define T_VENUS        (224.701*86400)      // orbit time in seconds

//
// typedefs
//

typedef struct {
    long double X;        // meters
    long double Y;
    long double VX;  // meters/secod
    long double VY;
    long double MASS;     // kg
} object_t; 

typedef struct {
    int64_t sim_alloc_size; 
    int64_t state_num;
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
static long double       sim_width;

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
    #define SIM_ALLOC_SIZE(n) (sizeof(sim_t) + (n) * sizeof(object_t))

    int32_t i, num_proc;
    int64_t sim_alloc_size;

    // print info msg
    INFO("initialize start\n");

    #define MAX_OBJECT 4

    // allocate sim
    free(sim);
    sim_alloc_size = SIM_ALLOC_SIZE(MAX_OBJECT);
    sim = calloc(1, sim_alloc_size);
    if (sim == NULL) {
        return -1;
    }

    // init sim  
    // XXX using XML for solar system, 
    //   - embed some of the XML in here
    //   - and get additional from web site
    object_t sun;
    sun.X    = 0;
    sun.Y    = 0;
    sun.VX   = 0;
    sun.VY   = 0;
    sun.MASS = M_SUN;
    sim->object[0] = sun;

    object_t earth;
    earth.X    = R_EARTH;
    earth.Y    = 0;
    earth.VX   = 0;
    earth.VY   = TWO_PI * R_EARTH / T_EARTH;
    earth.MASS = M_EARTH;
    sim->object[1] = earth;

#if MAX_OBJECT > 2
    object_t moon;
    moon.X    = earth.X - R_MOON;
    moon.Y    = earth.Y;
    moon.VX   = 0;
    moon.VY   = earth.VY + TWO_PI * R_MOON / T_MOON;
    moon.MASS = M_MOON;
    sim->object[2] = moon;
#endif

#if MAX_OBJECT > 3
    object_t venus;
    venus.X    = R_VENUS;
    venus.Y    = 0;
    venus.VX   = 0;
    venus.VY   = TWO_PI * R_VENUS / T_VENUS;
    venus.MASS = M_VENUS;
    sim->object[3] = venus;
#endif

    sim->sim_alloc_size = sim_alloc_size;
    sim->state_num  = 0;
    sim->max_object = MAX_OBJECT;

    // init control variables
    state = STATE_STOP;
    run_speed = DEFAULT_RUN_SPEED;
    sim_width = 2.1 * AU;

    // create worker threads
    num_proc = sysconf(_SC_NPROCESSORS_ONLN);
    max_thread = 1;  // xxx tbd
    INFO("max_thread=%d num_proc=%d\n", max_thread, num_proc);
    pthread_barrier_init(&barrier,NULL,max_thread);
    for (i = 0; i < max_thread; i++) {
        thread_cx_t * cx = malloc(sizeof(thread_cx_t));
        cx->id        = i;
        cx->first_obj = 0;
        cx->last_obj  = sim->max_object-1;
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
    uint8_t        r, g, b;
    char           str[100];
    bool           done = false;

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

    // draw objects 
    for (i = 0; i < sim->max_object; i++) {
        object_t * p = &sim->object[i];
        int32_t    win_x, win_y;

        win_x = (p->X + sim_width/2) * (simpane_width / sim_width);
        win_y = (p->Y + sim_width/2) * (simpane_width / sim_width);

#if 1
        int32_t win_r;
        SDL_Rect rect;

        wavelength_to_rgb((i%3) == 0 ? RED : (i%3) == 1 ? GREEN : BLUE,
                          &r, &g, &b);
        SDL_SetRenderDrawColor(sdl_renderer, r, g, b, SDL_ALPHA_OPAQUE);

        win_r = 2; // XXX use real value
        rect.x = win_x - win_r;
        rect.y = win_y - win_r;
        rect.w = 2 * win_r;
        rect.h = 2 * win_r;
        SDL_RenderFillRect(sdl_renderer, &rect);
#else
        SDL_RenderDrawPoint(sdl_renderer, win_x, win_y);
#endif
    }

    // clear ctlpane
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(sdl_renderer, &ctlpane);

    // display controls  
    // xxx clean all thi up
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

    int64_t days, hours, minutes, seconds;  // xxx use a routine
    seconds = sim->state_num * DT;
    days = seconds / 86400;
    seconds -= 86400 * days;
    hours = seconds / 3600;
    seconds -= hours * 3600;
    minutes = seconds / 60;
    seconds -= minutes * 60;
    sprintf(str, "%d %2.2d:%2.2d:%2.d", (int32_t)days, (int32_t)hours, (int32_t)minutes, (int32_t)seconds);
    sdl_render_text_font0(&ctlpane, 13, 0, str, SDL_EVENT_NONE);

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
        sim_gravity_terminate();  //xxx parms tbd, get the XML here
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
    #define PB(n)      (pb_buff + (n) * sim->sim_alloc_size)

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
            pb_buff = calloc(pb_max, sim->sim_alloc_size);
            if (pb_buff != NULL) {
                INFO("playback buff_size %d MB, pb_max=%d\n", (int32_t)((pb_max * sim->sim_alloc_size) / MB), pb_max);
                break;
            }
            pb_max -= 100;
            if (pb_max <= 0) {
                FATAL("failed allocate playback buffer\n");
            }
        }
        pb_end = pb_beg = pb_idx = 0;
        memcpy(PB(0), sim, sim->sim_alloc_size);
        pb_tail = 1;
    }
    if (max_thread > 1) {
        pthread_barrier_wait(&barrier);
    }

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
                memcpy(sim, PB((pb_tail-1)%pb_max), sim->sim_alloc_size);
            }
        }

        // all threads rendezvoud here
        if (max_thread > 1) {
            pthread_barrier_wait(&barrier);
        }

        // process based on curr_state
        if (curr_state == STATE_RUN) {
            int32_t i, k;

            // These are the simulation equations for updating an objects X position 
            // and X component of velocity. Similar equations are implemented in the 
            // following code for Y position and velocity.
            //
            // Where:
            //   EVALOBJ.X   object being evaluated X position
            //   EVALOBJ.VX  object being evaluated X velocity
            //   DT          simulation step time, for example 1 second works well for earth orbit
            //   G           gravitiational constant
            //   AX          X acceleration component of EVALOBJ
            //   Mi          mass of other object i
            //   XDISTi      X distance to other object i
            //   Ri          total distance to other object i
            //   DX          distance travelled by EVALOBJ in DT
            //   V1X         X velocity of EVALOBJ at begining of interval
            //   V2X         X velocity of EVALOBJ at end of interval
            // 
            //               Mi * XDISTi
            // AX = G * SUM(------------)
            //                  Ri^3
            //
            //                  1
            // DX = V1X * DT + --- * AX * DT^2
            //                  2
            //
            // V2X = sqrt(VIX^2 + 2 AX * ds)
            //
            // MYOBJ.X  = MYOBJ.X + DX
            // 
            // MYOBJ.VX = V2X
            //

            for (k = cx.first_obj; k <= cx.last_obj; k++) {
                object_t * EVALOBJ = &sim->object[k];
                long double SUMX, SUMY, Mi, XDISTi, YDISTi, Ri, AX, AY, DX, DY, V1X, V1Y, V2X, V2Y, tmp;

                SUMX = SUMY = 0;
                for (i = 0; i < sim->max_object; i++) {
                    if (i == k) continue;

                    object_t * OTHEROBJ = &sim->object[i];

                    Mi      = OTHEROBJ->MASS;
                    XDISTi  = OTHEROBJ->X - EVALOBJ->X;
                    YDISTi  = OTHEROBJ->Y - EVALOBJ->Y;
                    Ri      = sqrtl(XDISTi * XDISTi + YDISTi * YDISTi);
                    tmp     = Mi / (Ri * Ri * Ri);
                    SUMX    = SUMX + tmp * XDISTi;
                    SUMY    = SUMY + tmp * YDISTi;
                }

                AX = G * SUMX;
                AY = G * SUMY;

                V1X = EVALOBJ->VX;
                V1Y = EVALOBJ->VY;

                DX = V1X * DT + 0.5 * AX * DT * DT;
                DY = V1Y * DT + 0.5 * AY * DT * DT;

                V2X = sqrtl(V1X * V1X + 2.0 * AX * DX);
                V2Y = sqrtl(V1Y * V1Y + 2.0 * AY * DY);

                EVALOBJ->X  = EVALOBJ->X + DX;
                EVALOBJ->Y  = EVALOBJ->Y + DY;
                EVALOBJ->VX = DX < 0 ? -V2X : V2X;  // XXX
                EVALOBJ->VY = DY < 0 ? -V2Y : V2Y;
            }

            // all threads rendezvoud here
            if (max_thread > 1) {
                pthread_barrier_wait(&barrier);
            }

            // all particles have been updated, thread 0 does post processing
            if (cx.id == 0) {
                // increment the state_num
                sim->state_num++;

                // make playback copy
                memcpy(PB(pb_tail%pb_max), sim, sim->sim_alloc_size);
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

                memcpy(sim, PB(pb_idx), sim->sim_alloc_size);

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

                memcpy(sim, PB(pb_idx), sim->sim_alloc_size);

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

    if (max_thread > 1) {
        pthread_barrier_wait(&barrier);
    }
    INFO("thread terminating, id=%d\n", cx.id);
    return NULL;
}
