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
#define SDL_EVENT_SHRINK         (SDL_EVENT_USER_START + 4)
#define SDL_EVENT_RESTORE        (SDL_EVENT_USER_START + 5)
#define SDL_EVENT_PLAYBACK_REV   (SDL_EVENT_USER_START + 6)
#define SDL_EVENT_PLAYBACK_FWD   (SDL_EVENT_USER_START + 7)
#define SDL_EVENT_RESET          (SDL_EVENT_USER_START + 9)
#define SDL_EVENT_RESET_PARAMS   (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_BACK           (SDL_EVENT_USER_START + 11)

#define PARTICLE_DIAMETER ((int64_t)(1000000))

#define SPEED_AVG                  524  // Kelvin = 70 deg F
#define SPEED_SPREAD               20

#define SPEED_TO_COLOR(s) \
    ((s) <  SPEED_AVG - SPEED_SPREAD/2 ? PURPLE : \
     (s) <  SPEED_AVG - SPEED_SPREAD/6 ? BLUE   : \
     (s) <= SPEED_AVG + SPEED_SPREAD/6 ? GREEN  : \
     (s) <= SPEED_AVG + SPEED_SPREAD/2 ? YELLOW   \
                                       : RED)

#define CONT_SHRINK_RESTORE_SPEED  10

#define DEFAULT_MAX_PARTICLE  1000
#define MAX_MAX_PARTICLE      10000
#define MIN_MAX_PARTICLE      1

#define DEFAULT_SIM_WIDTH     100    // units is PARTICLE_DIAMETER
#define MAX_SIM_WIDTH         10000
#define MIN_SIM_WIDTH         3

#define STATE_STOP               0
#define STATE_LOW_ENTROPY_STOP   1
#define STATE_RUN                2
#define STATE_PLAYBACK_REV       3
#define STATE_PLAYBACK_FWD       4
#define STATE_PLAYBACK_STOP      5
#define STATE_TERMINATE_THREADS  9

#define STATE_STR(s) \
    ((s) == STATE_STOP              ? "STOP"              : \
     (s) == STATE_LOW_ENTROPY_STOP  ? "LOW_ENTROPY_STOP"  : \
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
    int64_t x;
    int64_t y;
    double  speed;
    double  x_speed;
    double  y_speed;
} particle_t;

typedef struct {
    int64_t state_num;
    int64_t sim_width;
    int64_t cont_width;
    int32_t max_particle;
    particle_t particle[0];
} sim_t;

//
// variables
//

static sim_t           * sim;

static int32_t           state; 
static int32_t           run_speed;
static int32_t           cont_shrink_restore_speed;

static int32_t           max_thread;
static pthread_t         thread_id[MAX_THREAD];
static pthread_barrier_t barrier;

// 
// prototypes
//

int32_t sim_container_init(bool startup, int32_t max_particle, int64_t sim_width);
void sim_container_terminate(void);
bool sim_container_display(void);
void * sim_container_thread(void * cx);
bool sim_container_should_auto_stop(void);

// -----------------  MAIN  -----------------------------------------------------

void sim_container(void) 
{
    if (sim_container_init(true, DEFAULT_MAX_PARTICLE, DEFAULT_SIM_WIDTH*PARTICLE_DIAMETER) != 0) {
        return;
    }

    while (true) {
        bool done = sim_container_display();
        if (done) {
            break;
        }
    }

    sim_container_terminate();
}

// -----------------  INIT & TERMINATE  -----------------------------------------

int32_t sim_container_init(bool startup, int32_t max_particle, int64_t sim_width)
{
    #define SIM_SIZE(n) (sizeof(sim_t) + (n) * sizeof(particle_t))

    int32_t i, num_proc, direction;

    // print info msg
    INFO("initialize start, max_particle=%d sim_width=%"PRId64"\n",max_particle, sim_width);

    // allocate sim
    free(sim);
    sim = calloc(1, SIM_SIZE(max_particle));
    if (sim == NULL) {
        return -1;
    }

    // set simulation state
    sim->state_num    = 0;
    sim->sim_width    = sim_width;
    sim->cont_width   = sim_width;
    sim->max_particle = max_particle;
    for (i = 0; i < max_particle; i++) {
        particle_t * p = &sim->particle[i];
        p->x     = 0;
        p->y     = 0;
        p->speed = SPEED_AVG + random_triangular(-SPEED_SPREAD/2, SPEED_SPREAD/2);

        direction = random_uniform(0,1000000-1);
        p->x_speed   = p->speed * cos(direction * (M_PI / (1000000/2)));
        p->y_speed   = p->speed * sin(direction * (M_PI / (1000000/2)));
    }

    // init control variables
    state = STATE_STOP;
    if (startup) {
        run_speed = DEFAULT_RUN_SPEED;
    }
    cont_shrink_restore_speed = 0;

    // create worker threads
    num_proc = sysconf(_SC_NPROCESSORS_ONLN);
    max_thread = (num_proc == 1            ? 1 :
                  num_proc <= MAX_THREAD+1 ? num_proc-1
                                           : MAX_THREAD);
    INFO("max_thread=%d num_proc=%d\n", max_thread, num_proc);
    pthread_barrier_init(&barrier,NULL,max_thread);
    for (i = 0; i < max_thread; i++) {
        pthread_create(&thread_id[i], NULL, sim_container_thread, (void*)(long)i);
    }

    // success
    INFO("initialize complete\n");
    return 0;
}

void sim_container_terminate(void)
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

bool sim_container_display(void)
{
    SDL_Rect      ctlpane;
    SDL_Rect      cont_rect;
    sdl_event_t * event;
    int32_t       i, simpane_width, cont_width, count, color;
    int32_t       win_x, win_y, win_r;
    double        sum_speed, temperature;
    char          str[100];
    bool          done = false;

    static int32_t       win_r_last = -1;
    static SDL_Texture * circle_texture[MAX_COLOR];       
    
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

    // determine display radius of particles,
    // if radius has changed then recompute the circle textures
    win_r = simpane_width / (sim->sim_width/PARTICLE_DIAMETER) / 2;
    if (win_r < 1) {
        win_r = 1;
    }
    if (win_r != win_r_last) {
        for (color = 0; color < MAX_COLOR; color++) {
            if (circle_texture[color] != NULL) {
                SDL_DestroyTexture(circle_texture[color]);
                circle_texture[color] = NULL;
            }
            circle_texture[color] = sdl_create_filled_circle_texture(win_r, sdl_pixel_rgba[color]);
        }
        win_r_last = win_r;
    }

    // if in a stopped state then delay for 50 ms, to reduce cpu usage
    count = 0;
    while ((state == STATE_STOP ||
            state == STATE_LOW_ENTROPY_STOP ||
            state == STATE_PLAYBACK_STOP) &&
           (count < 100))
    {
        count++;
        usleep(1000);
    }

    // clear window
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);

    // draw particle
    sum_speed = 0;
    for (i = 0; i < sim->max_particle; i++) {
        particle_t * p = &sim->particle[i];

        win_x = (p->x + sim->sim_width/2) * simpane_width / sim->sim_width;
        win_y = (p->y + sim->sim_width/2) * simpane_width / sim->sim_width;
        sdl_render_circle(win_x, win_y, circle_texture[SPEED_TO_COLOR(p->speed)]);

        sum_speed += p->speed;
    }
    temperature = sum_speed / sim->max_particle - 454;

    // clear ctlpane
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(sdl_renderer, &ctlpane);

    // display controls
    sdl_render_text_font0(&ctlpane,  0, 3,  "CONTAINER",       SDL_EVENT_NONE);
    sdl_render_text_font0(&ctlpane,  2, 0,  "RUN",             SDL_EVENT_RUN);
    sdl_render_text_font0(&ctlpane,  2, 9,  "STOP",            SDL_EVENT_STOP);
    sdl_render_text_font0(&ctlpane,  4, 0,  "SLOW",            SDL_EVENT_SLOW);
    sdl_render_text_font0(&ctlpane,  4, 9,  "FAST",            SDL_EVENT_FAST);
    sdl_render_text_font0(&ctlpane,  6, 0,  "SHRINK",          SDL_EVENT_SHRINK);
    sdl_render_text_font0(&ctlpane,  6, 9,  "RESTORE",         SDL_EVENT_RESTORE);
    sdl_render_text_font0(&ctlpane,  8, 0,  "PB_REV",          SDL_EVENT_PLAYBACK_REV);
    sdl_render_text_font0(&ctlpane,  8, 9,  "PB_FWD",          SDL_EVENT_PLAYBACK_FWD);
    sdl_render_text_font0(&ctlpane, 10, 0,  "RESET",           SDL_EVENT_RESET);
    sdl_render_text_font0(&ctlpane, 10, 9,  "RESET_PARAM",     SDL_EVENT_RESET_PARAMS);
    sdl_render_text_font0(&ctlpane, 18,15,  "BACK",            SDL_EVENT_BACK);

    // display status line
    sprintf(str, "%s %"PRId64" ", STATE_STR(state), sim->state_num);
    if (state == STATE_RUN || state == STATE_STOP || state == STATE_LOW_ENTROPY_STOP) {
        if (cont_shrink_restore_speed < 0) {
            strcat(str, "SHRINK");
        } else if (cont_shrink_restore_speed > 0) {
            strcat(str, "RESTORE");
        }
    }
    sdl_render_text_font0(&ctlpane, 12, 0, str, SDL_EVENT_NONE);

    // display remaining status lines 
    sprintf(str, "PARTICLES   = %d", sim->max_particle);    
    sdl_render_text_font0(&ctlpane, 13, 0, str, SDL_EVENT_NONE);
    sprintf(str, "SIM_WIDTH   = %"PRId64, sim->sim_width / PARTICLE_DIAMETER);    
    sdl_render_text_font0(&ctlpane, 14, 0, str, SDL_EVENT_NONE);
    sprintf(str, "RUN_SPEED   = %d", run_speed);    
    sdl_render_text_font0(&ctlpane, 15, 0, str, SDL_EVENT_NONE);
    sprintf(str, "TEMPERATURE = %0.2f", temperature);
    sdl_render_text_font0(&ctlpane, 16, 0, str, SDL_EVENT_NONE);

    // draw container border
    cont_width = (int64_t)simpane_width * sim->cont_width / sim->sim_width;
    cont_rect.x = (simpane_width - cont_width) / 2;
    cont_rect.y = (simpane_width - cont_width) / 2;
    cont_rect.w = cont_width;
    cont_rect.h = cont_width;
    sdl_render_rect(&cont_rect, 3, sdl_pixel_rgba[GREEN]);

    // present the display
    SDL_RenderPresent(sdl_renderer);

    // handle events
    event = sdl_poll_event();
    if (event->event != SDL_EVENT_NONE) {
        sdl_play_event_sound();
    }
    switch (event->event) {
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
    case SDL_EVENT_SHRINK:
        cont_shrink_restore_speed = -CONT_SHRINK_RESTORE_SPEED;
        break;
    case SDL_EVENT_RESTORE:
        cont_shrink_restore_speed = CONT_SHRINK_RESTORE_SPEED;
        break;
    case SDL_EVENT_PLAYBACK_REV:
        state = STATE_PLAYBACK_REV;
        break;
    case SDL_EVENT_PLAYBACK_FWD:
        state = STATE_PLAYBACK_FWD;
        break;
    case SDL_EVENT_RESET:
    case SDL_EVENT_RESET_PARAMS: {
        int32_t mp = sim->max_particle;
        int32_t sw = sim->sim_width / PARTICLE_DIAMETER;

        sim_container_terminate();
        if (event->event == SDL_EVENT_RESET_PARAMS) {
            char cur_mp[100], cur_sw[100];
            char ret_mp[100], ret_sw[100];

            sprintf(cur_mp, "%d", mp);
            sprintf(cur_sw, "%d", sw);
            sdl_get_string(2, "PARTICLES", cur_mp, ret_mp, "SIM_WIDTH", cur_sw, ret_sw);
            sscanf(ret_mp, "%d", &mp);
            sscanf(ret_sw, "%d", &sw);

            if (mp > MAX_MAX_PARTICLE) mp = MAX_MAX_PARTICLE;
            if (mp < MIN_MAX_PARTICLE) mp = MIN_MAX_PARTICLE;
            if (sw > MAX_SIM_WIDTH) sw = MAX_SIM_WIDTH;
            if (sw < MIN_SIM_WIDTH) sw = MIN_SIM_WIDTH;
        }
        sim_container_init(false, mp, sw * PARTICLE_DIAMETER);
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

void * sim_container_thread(void * cx)
{
    #define BATCH_SIZE 100 
    #define PB(n)      (pb_buff + (n) * sim_size)

    int32_t         id = (long)cx;

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

                    // if the new position is within the container then
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
                    // sides of the container; note that if the particle goes off
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
                if (sim_container_should_auto_stop()) {
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

    pthread_barrier_wait(&barrier);
    INFO("thread terminating, id=%d\n", id);
    return NULL;
}

// -----------------  SUPPORT ROUTINES  -----------------------------------------

bool sim_container_should_auto_stop(void)
{
    int32_t i;
    bool auto_stop;
    static int64_t last_auto_stop_state_num;

    if (sim->state_num <= 1) {
        last_auto_stop_state_num = 0;
    }

    if (sim->max_particle == 1 ||
        sim->state_num < last_auto_stop_state_num + 100) 
    {
        return false;
    }

    for (i = 0; i < sim->max_particle; i++) {
        particle_t * p = &sim->particle[i];
        if (abs(p->x) > PARTICLE_DIAMETER/4 || abs(p->y) > PARTICLE_DIAMETER/4) {
            break;
        }
    }

    auto_stop = (i == sim->max_particle);
    if (auto_stop) {
        last_auto_stop_state_num = sim->state_num;
    }

    return auto_stop;
}
