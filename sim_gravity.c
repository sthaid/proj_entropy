// XXX otimize
// - multiple threads,
// - don't use barrier
// - if forces are not significant then don't inspect every time
// - enable / disable display track
// - select DT
// - include DT in file too, with different values for android

// XXX hover on planet to diplay name, or enable tracking, or enable trails

// XXX add other stars

// XXX display width in light minutes or seconds

// XXX use different colors for planets

// XXX add more moons

// 
// xxx DELETE
// Distance to Pluto = 
//   13 light hours
//   1.4E13 meters
//   1.4E16 mm
// 2^63 = 9E18
//

// xxx add select for DT

// xxx reset simpane to center and also default width ??

// ----------------------------------------------------------------------------------------------------
// -----------------  BEGIN  --------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

#include "util.h"

//
// NOTES:
//
// * Size of floating point, in bytes, on 64bit Fedora20 Linux, and Android
//                DOUBLE      LONG DOUBLE
//     LINUX         8            10 
//     ANDROID       8            8
//
// * Range of Floating point types:
//     float        4 byte    1.2E-38   to 3.4E+38      6 decimal places
//     double       8 byte    2.3E-308  to 1.7E+308    15 decimal places
//     long double 10 byte    3.4E-4932 to 1.1E+4932   19 decimal places
//

//
// defines
//

#define MAX_THREAD 16
#define TWO_PI     (2.0*M_PI)
#define G          6.673E-11  // mks units  

#ifndef ANDROID
  #define DT       5.0    
#else
  #define DT       5.0   
#endif

#define SDL_EVENT_STOP                   (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN                    (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_SELECT                 (SDL_EVENT_USER_START + 2)
#define SDL_EVENT_SIMPANE_ZOOM_IN        (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_SIMPANE_ZOOM_OUT       (SDL_EVENT_USER_START + 11)
#define SDL_EVENT_SIMPANE_MOUSE_CLICK    (SDL_EVENT_USER_START + 12)
#define SDL_EVENT_SIMPANE_MOUSE_MOTION   (SDL_EVENT_USER_START + 13)
#define SDL_EVENT_TRACKERPANE_ZOOM_IN    (SDL_EVENT_USER_START + 20)
#define SDL_EVENT_TRACKERPANE_ZOOM_OUT   (SDL_EVENT_USER_START + 21)
#define SDL_EVENT_BACK                   (SDL_EVENT_USER_START + 30)

#define STATE_STOP               0
#define STATE_RUN                1
#define STATE_TERMINATE_THREADS  9

#define STATE_STR(s) \
    ((s) == STATE_STOP              ? "STOP"              : \
     (s) == STATE_RUN               ? "RUN"               : \
     (s) == STATE_TERMINATE_THREADS ? "TERMINATE_THREADS"   \
                                    : "????")

#define CURR_DISPLAY_SIMULATION 0
#define CURR_DISPLAY_SELECTION  1

#ifdef ANDROID
    #define sqrtl sqrt
#endif

#define long   //XXX temp

//
// typedefs
//

typedef struct {
    long double X;        // meters
    long double Y;
    long double VX;       // meters/secod xxx maybe use int64_t
    long double VY;
    long double MASS;     // kg
    long double RADIUS;   // meters
    int32_t     DISP_COLOR;
    int32_t     DISP_R; 
} object_t; 

typedef struct {
    long double sim_time;
    int32_t     max_object;
    object_t    object[0];
} sim_t;

//
// variables
//

static sim_t           * sim;

static int32_t           state; 
static long double       sim_width;
static long double       sim_x;
static long double       sim_y;
static long double       tracker_width;
static int32_t           tracker_obj;

static long double       run_start_sim_time;
static long double       run_start_wall_time;

static int32_t           max_thread;
static pthread_t         thread_id[MAX_THREAD];

static int32_t           curr_display;

// 
// prototypes
//

int32_t sim_gravity_init(void);
void sim_gravity_terminate(void);
int32_t sim_gravity_init_sim_from_file(char * filename);
void sim_gravity_set_obj_disp_r(sim_t * new_sim);
bool sim_gravity_display(void);
bool sim_gravity_display_simulation(bool new_display);
bool sim_gravity_display_selection(bool new_display);
void * sim_gravity_thread(void * cx);
int32_t sim_gravity_rendezvoud(int32_t thread_id);

// -----------------  MAIN  -----------------------------------------------------

void sim_gravity(void) 
{
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

    int32_t i, num_proc, ret;

    // print info msg
    INFO("initialize start\n");

    #define MAX_OBJECT 4

    // init sim  
    ret = sim_gravity_init_sim_from_file("solar_system");  // xxx DEFAULT_SIM_FILE
    INFO("XXX RET %d\n", ret);
    // exit(1); //XXX


    // init control variables
    // XXX include sim_width and tracker_width in file, and sim_x, sim_y, tracker_obj
    #define AU 1.495978707E11       // meters  XXX delete
    state = STATE_STOP;
    sim_width = 2.1 * AU;
    sim_x = 0;
    sim_y = 0;
    tracker_width = 2000000000; 
    tracker_obj = -1;

    // create worker threads
    num_proc = sysconf(_SC_NPROCESSORS_ONLN);
    //max_thread = 2;  // xxx tbd
    max_thread = num_proc - 1;
    INFO("max_thread=%d num_proc=%d\n", max_thread, num_proc);
    for (i = 0; i < max_thread; i++) {
        pthread_create(&thread_id[i], NULL, sim_gravity_thread, (void*)(int64_t)i);
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
    free(sim);
    sim = NULL;

    // done
    INFO("terminate complete\n");
}

// -----------------  SIM GRAVITY INIT  -----------------------------------------

int32_t sim_gravity_init_sim_from_file(char * filename) 
{
    #define MAX_BUFF 1000000  //xxx get real filesize
    #define MAX_RELTO 10      //xxx check overflow

    typedef struct {
        long double X;
        long double Y;
        long double VX;
        long double VY;
    } relto_t;

    char        pathname[200];
    int32_t     len, i, cnt;
    char      * s, * s_next, * tmp;
    relto_t     relto[MAX_RELTO];
    long double X, Y, VX, VY, MASS, RADIUS;
    char        NAME[100], COLOR[100];

    SDL_RWops * rw = NULL;
    char      * buff = NULL;
    int32_t     ret = 0;
    int32_t     line = 0;
    sim_t     * new_sim = NULL;
    int32_t     max_object = 0;
    int32_t     alloced_object = 0;
    int32_t     idx = 0;
    int32_t     last_idx = 0;

    bzero(relto, sizeof(relto));

    // read file to memory buffer
    buff = malloc(MAX_BUFF);
    bzero(buff, MAX_BUFF);
    sprintf(pathname, "%s/%s", "sim_gravity", filename);
    rw = SDL_RWFromFile(pathname, "r");
    if (rw == NULL) {
        ERROR("open %s, %s\n", pathname, SDL_GetError());
        ret = -1;
        goto done;
    }
    len = SDL_RWread(rw, buff, 1, MAX_BUFF-1);
    if (len == 0) {
        ERROR("read %s, %s\n", pathname, SDL_GetError());
        ret = -1;
        goto done;
    }

    // read lines from buff
    for (s = buff; *s; s = s_next) {
        // find newline and replace with 0, and set s_next
        tmp = strchr(s, '\n');
        if (tmp != NULL) {
            *tmp = 0;
            s_next = tmp + 1;
        } else {
            s_next = s + strlen(s);
        }
        line++;

        // if line begins with '#', or is blank then skip
        if (s[0] == '#') {
            continue;
        }
        for (i = 0; s[i] == ' '; i++) {
            ;
        }
        if (s[i] == '\0') {
            continue;
        }

        // scan this line
        cnt = sscanf(s, "%s %lf %lf %lf %lf %lf %lf %s",
                     NAME, &X, &Y, &VX, &VY, &MASS, &RADIUS, COLOR);
        if (cnt != 8) {
            ERROR("line %d invalid in file %s\n", line, filename);
            ret = -1;
            goto done;
        }
        // INFO("XXX - %s %lf %lf %lf %lf %lf %lf %s",
             // NAME, X, Y, VX, VY, MASS, RADIUS, COLOR);

        // allocate more memory for new_sim, if needed
        max_object++;
        if (max_object >= alloced_object) {
            alloced_object += 100;
            new_sim = realloc(new_sim, SIM_ALLOC_SIZE(alloced_object));
            if (new_sim == NULL) {
                ERROR("failed realloc new_sim, alloced_object %d\n", alloced_object);
                ret = -1;
                goto done;
            }
        }

        // determine idx for accessing relative_to[]
        for (idx = 0; s[idx] == ' '; idx++) ;
        idx++;
        if (idx > last_idx+1) {
            ERROR("invalid idx %d, last_idx=%d line=%d\n", idx, last_idx, line);
            ret = -1;
            goto done;
        }
        last_idx = idx;
        INFO("XXX %s %d\n", NAME, idx);

        // initialize new_sim->object
        // XXX make use of name
        object_t * obj = &new_sim->object[max_object-1];
        obj->X          = X + relto[idx-1].X;
        obj->Y          = Y + relto[idx-1].Y;
        obj->VX         = VX + relto[idx-1].VX;
        obj->VY         = VY + relto[idx-1].VY;
        obj->MASS       = MASS;
        obj->RADIUS     = RADIUS;
        obj->DISP_COLOR = COLOR_STR_TO_COLOR(COLOR);
        obj->DISP_R     = 5;  // filled in by call to  sim_gravity_set_obj_disp_r below

        // save new relto
        relto[idx].X  = obj->X;
        relto[idx].Y  = obj->Y;
        relto[idx].VX = obj->VX;
        relto[idx].VY = obj->VY;
    }

    // fill in new_sim header fields
    new_sim->sim_time = 0;
    new_sim->max_object = max_object;

    // fill in DISP_R fields
    // XXX this should be changed to use RADIUS
    // XXX better name for vvv
    sim_gravity_set_obj_disp_r(new_sim);

    // XXX prints
    INFO("-----------------------\n");
    for (i = 0; i < new_sim->max_object; i++) {
        object_t * obj = new_sim->object+i;
        INFO("X=%lf Y=%lf XV=%lf YV=%lf MAAA=%lf RADIUS=%lf DISP_COLOR=%d DIR_R=%d\n",
            obj->X, obj->Y, obj->VX, obj->VY, obj->MASS, obj->RADIUS, obj->DISP_COLOR, obj->DISP_R);
    }
    INFO("-----------------------\n");

    // success
    free(sim);
    sim = new_sim;
    ret = 0;

done:
    // cleanup and return
    if (rw != NULL) {
        SDL_RWclose(rw);
    }
    if (ret != 0) {
        free(new_sim);
    }
    free(buff);

    return ret;
}

void sim_gravity_set_obj_disp_r(sim_t * new_sim)
{
    long double radius_raw[new_sim->max_object]; 
    long double min_radius_raw, max_radius_raw, min_disp_r, max_disp_r, delta;
    int32_t i;

    // xxx redo to not rely on mass but on actual diamter

    #define MIN_DISP_R (2.)
    #define MAX_DISP_R (5. * MIN_DISP_R)
    #define CENTER_DISP_R ((MAX_DISP_R + MIN_DISP_R) / 2.)

    // fill in radius raw values with cube root of mass
    for (i = 0; i < new_sim->max_object; i++) {
        radius_raw[i] = exp(log(new_sim->object[i].MASS) / 3);
    }

    // determine min and max raw radius values
    min_radius_raw = 1E99;
    max_radius_raw = -1E99;
    for (i = 0; i < new_sim->max_object; i++) {
        if (radius_raw[i] < min_radius_raw) {
            min_radius_raw = radius_raw[i];
        }
        if (radius_raw[i] > max_radius_raw) {
            max_radius_raw = radius_raw[i];
        }
    }
    INFO("MIN/MAX RADIUS RAW  %lf %lf\n", min_radius_raw, max_radius_raw);  // xxx delete prints in here

    // if min and max equal then
    //   set obj_radius to CENTER_DISP_R
    if (min_radius_raw == max_radius_raw) {
        INFO("CASE 1 - EQUAL\n");
        for (i = 0; i < new_sim->max_object; i++) {
            new_sim->object[i].DISP_R = CENTER_DISP_R;
        }

    // else if max / min < 5 then
    //   map raw radius values to range min_disp_r to max_disp_r,
    //   where these values have ratio equal to max_radius_raw/min_radius_raw
    } else if (max_radius_raw / min_radius_raw <= 5) {
        INFO("CASE 2 - LESS THAN 10\n");
        delta = (max_radius_raw / min_radius_raw * CENTER_DISP_R - CENTER_DISP_R) / 
                (max_radius_raw  / min_radius_raw + 1);
        min_disp_r = CENTER_DISP_R - delta;
        max_disp_r = CENTER_DISP_R + delta;

        for (i = 0; i < new_sim->max_object; i++) {
            new_sim->object[i].DISP_R = (max_disp_r - min_disp_r) / (max_radius_raw - min_radius_raw) * 
                                    (radius_raw[i] - min_radius_raw ) + min_disp_r;
        }

    // else 
    //   convert raw radius values to log scale
    //   map these new raw radius values to MIN_DISP_R to MAX_DISP_R
    } else {
        INFO("CASE 3 - \n");
        for (i = 0; i < new_sim->max_object; i++) {
            radius_raw[i] = log(radius_raw[i]);
        }
        for (i = 0; i < new_sim->max_object; i++) {
            INFO("NEW %d = %lf\n", i, radius_raw[i]);
        }

        min_radius_raw = 1E99;
        max_radius_raw = -1E99;
        for (i = 0; i < new_sim->max_object; i++) {
            if (radius_raw[i] < min_radius_raw) {
                min_radius_raw = radius_raw[i];
            }
            if (radius_raw[i] > max_radius_raw) {
                max_radius_raw = radius_raw[i];
            }
        }
        // XXX INFO("NEW MIN/MAX RADIUS RAW  %lf %lf\n", min_radius_raw, max_radius_raw);

        for (i = 0; i < new_sim->max_object; i++) {
            new_sim->object[i].DISP_R = (MAX_DISP_R - MIN_DISP_R) / (max_radius_raw - min_radius_raw) * 
                                    (radius_raw[i] - min_radius_raw) + MIN_DISP_R;
        }
    }

    // xxx, and search for too many other INOFO prints
    for (i = 0; i < new_sim->max_object; i++) {
        INFO("RESULT %d = %d\n", i, new_sim->object[i].DISP_R);
    }
}

// -----------------  DISPLAY  --------------------------------------------------

bool sim_gravity_display(void)
{
    bool done, new_display;
    static int32_t curr_display_last = -1;

    new_display = (curr_display != curr_display_last);
    curr_display_last = curr_display;

    if (curr_display == CURR_DISPLAY_SIMULATION) {
        done = sim_gravity_display_simulation(new_display);
    } else {
        done = sim_gravity_display_selection(new_display);
    }

    if (done) {
        curr_display_last = -1;
    }

    return done;
}

bool sim_gravity_display_simulation(bool new_display)
{
    #define IN_RECT(X,Y,RECT) \
        ((X) >= (RECT).x && (X) < (RECT).x + (RECT).w && \
         (Y) >= (RECT).y && (Y) < (RECT).y + (RECT).h)

    #define MAX_CIRCLE_RADIUS 51
    #define MAX_CIRCLE_COLOR  (MAX_COLOR+1)

    SDL_Rect       ctl_pane, sim_pane, tracker_pane;
    int32_t        sim_pane_width, tracker_pane_width, i, row, col;
    sdl_event_t *  event;
    char           str[100], str1[100];
    bool           done = false;

    static SDL_Texture * circle_texture_tbl[MAX_CIRCLE_RADIUS][MAX_CIRCLE_COLOR];

    //
    // if this display has just been activated then 
    // nothing to do here 
    //

    if (new_display) {
        ;
    }

    //
    // initialize the pane locations:
    // - sim_pane: the main simulation display area
    // - tracker_pane: the tracker display area
    // - ctl_pane: the controls display area
    //

    if (sdl_win_width > sdl_win_height) {
        int32_t ctl_pane_width, ctl_pane_height;
        sim_pane_width = sdl_win_height;
        ctl_pane_width = sdl_win_width - sim_pane_width, 
        ctl_pane_height = sdl_win_height - ctl_pane_width;
        tracker_pane_width = ctl_pane_width;
        SDL_INIT_PANE(sim_pane, 
                      0, 0,  
                      sim_pane_width, sim_pane_width);
        SDL_INIT_PANE(tracker_pane, 
                      sim_pane_width, ctl_pane_height,
                      tracker_pane_width, tracker_pane_width);
        SDL_INIT_PANE(ctl_pane, 
                      sim_pane_width, 0, 
                      ctl_pane_width, ctl_pane_height);
    } else {
        sim_pane_width = sdl_win_width;
        tracker_pane_width = 0;
        SDL_INIT_PANE(sim_pane, 0, 0,  sim_pane_width, sim_pane_width);
        SDL_INIT_PANE(tracker_pane, 0, 0, 0, 0);
        SDL_INIT_PANE(ctl_pane, 0, 0, 0, 0);
    }
    sdl_event_init();

    //
    // if in stopped state then delay a bit, to reduce cpu usage
    //

    //if (state == STATE_STOP) {  //XXX
        usleep(5000);
    //}

    //
    // clear window
    //

    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);

    //
    // SIMPANE ...
    //

    // draw sim_pane objects 
    for (i = 0; i < sim->max_object; i++) {
        object_t * p = &sim->object[i];
        int32_t    disp_x, disp_y, disp_r, disp_color;

        disp_x     = sim_pane.x + (p->X - sim_x + sim_width/2) * (sim_pane_width / sim_width);
        disp_y     = sim_pane.y + (p->Y - sim_y + sim_width/2) * (sim_pane_width / sim_width);
        disp_r     = p->DISP_R;
        disp_color = p->DISP_COLOR;

        if (i == tracker_obj) { 
            disp_color = RED;
        }

        if (IN_RECT(disp_x, disp_y, sim_pane) == false) {
            continue;
        }

        if (disp_r < 1) disp_r = 1;
        if (disp_r >= MAX_CIRCLE_RADIUS) disp_r = MAX_CIRCLE_RADIUS-1;
        if (disp_color >= MAX_CIRCLE_COLOR) disp_color = MAX_CIRCLE_COLOR-1;

        if (circle_texture_tbl[disp_r][disp_color] == NULL) {
            circle_texture_tbl[disp_r][disp_color] =
                sdl_create_filled_circle_texture(disp_r, sdl_pixel_rgba[disp_color]);
        }
        sdl_render_circle(disp_x, disp_y, circle_texture_tbl[disp_r][disp_color]);
    }

    // display sim_pane title
    sprintf(str, "WIDTH %0.2lf AU", sim_width/AU);
    sdl_render_text_font0(&sim_pane,  0, 0,  str, SDL_EVENT_NONE);

    // register sim_pane controls
    col = SDL_PANE_COLS(&sim_pane,0)-2;
    sdl_render_text_font0(&sim_pane,  0, col,  "+", SDL_EVENT_SIMPANE_ZOOM_IN);
    sdl_render_text_font0(&sim_pane,  2, col,  "-", SDL_EVENT_SIMPANE_ZOOM_OUT);
    sdl_event_register(SDL_EVENT_SIMPANE_MOUSE_CLICK, SDL_EVENT_TYPE_MOUSE_CLICK, &sim_pane);
    sdl_event_register(SDL_EVENT_SIMPANE_MOUSE_MOTION, SDL_EVENT_TYPE_MOUSE_MOTION, &sim_pane);

    // draw sim_pane border
    sdl_render_rect(&sim_pane, 3, sdl_pixel_rgba[GREEN]);

    //
    // TRACKERPANE ...
    //

    // draw tracker_pane objects 
    if (tracker_obj != -1) {
        long double tracker_x = sim->object[tracker_obj].X;
        long double tracker_y = sim->object[tracker_obj].Y;
        for (i = 0; i < sim->max_object; i++) {
            object_t * p = &sim->object[i];
            int32_t    disp_x, disp_y, disp_r, disp_color;

            disp_x     = tracker_pane.x + (p->X - tracker_x + tracker_width/2) * (tracker_pane_width / tracker_width);
            disp_y     = tracker_pane.y + (p->Y - tracker_y + tracker_width/2) * (tracker_pane_width / tracker_width);
            disp_r     = p->DISP_R;
            disp_color = p->DISP_COLOR;

            if (IN_RECT(disp_x, disp_y, tracker_pane) == false) {
                continue;
            }

            if (disp_r < 1) disp_r = 1;
            if (disp_r >= MAX_CIRCLE_RADIUS) disp_r = MAX_CIRCLE_RADIUS-1;
            if (disp_color >= MAX_CIRCLE_COLOR) disp_color = MAX_CIRCLE_COLOR-1;

            if (circle_texture_tbl[disp_r][disp_color] == NULL) {
                circle_texture_tbl[disp_r][disp_color] =
                    sdl_create_filled_circle_texture(disp_r, sdl_pixel_rgba[disp_color]);
            }
            sdl_render_circle(disp_x, disp_y, circle_texture_tbl[disp_r][disp_color]);
        }
    } else {
        row = SDL_PANE_ROWS(&tracker_pane,0) / 2 - 1;
        sdl_render_text_ex(&tracker_pane, row, 0, "NOT", SDL_EVENT_NONE, SDL_FIELD_COLS_UNLIMITTED, true, 0);
        sdl_render_text_ex(&tracker_pane, row+1, 0, "SELECTED", SDL_EVENT_NONE, SDL_FIELD_COLS_UNLIMITTED, true, 0);
    }

    // register tracker_pane controls
    col = SDL_PANE_COLS(&tracker_pane,0)-2;
    sdl_render_text_font0(&tracker_pane,  0, col,  "+", SDL_EVENT_TRACKERPANE_ZOOM_IN);
    sdl_render_text_font0(&tracker_pane,  2, col,  "-", SDL_EVENT_TRACKERPANE_ZOOM_OUT);

    // display tracker_pane title
    sprintf(str, "WIDTH %0.2lf AU", tracker_width/AU);
    sdl_render_text_font0(&tracker_pane,  0, 0,  str, SDL_EVENT_NONE);

    // draw trakerpane border
    sdl_render_rect(&tracker_pane, 3, sdl_pixel_rgba[GREEN]);

    //
    // CTLPANE ...
    //

    // clear ctl_pane
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(sdl_renderer, &ctl_pane);

    // display controls  
    sdl_render_text_font0(&ctl_pane,  0, 0,  "GRAVITY", SDL_EVENT_NONE);
    sdl_render_text_font0(&ctl_pane,  0,15,  "BACK",    SDL_EVENT_BACK);
    sdl_render_text_font0(&ctl_pane,  2, 0,  "RUN",     SDL_EVENT_RUN);
    sdl_render_text_font0(&ctl_pane,  2, 9,  "STOP",    SDL_EVENT_STOP);
    sdl_render_text_font0(&ctl_pane,  4, 0,  "SELECT",  SDL_EVENT_SELECT);

    // display status lines
    sprintf(str, "%-4s %s", STATE_STR(state), dur2str(str1,sim->sim_time));
    sdl_render_text_font0(&ctl_pane, 6, 0, str, SDL_EVENT_NONE);

    if (state == STATE_RUN) {
        int32_t perf = (sim->sim_time - run_start_sim_time) / ((long double)microsec_timer()/1000000.0 - run_start_wall_time);
        sprintf(str, "%d", perf);
        sdl_render_text_font0(&ctl_pane, 4, 9, str, SDL_EVENT_NONE);
    }

    //
    // present the display
    //

    SDL_RenderPresent(sdl_renderer);

    //
    // handle events
    //

    event = sdl_poll_event();
    switch (event->event) {
    case SDL_EVENT_RUN:
        state = STATE_RUN;
        run_start_sim_time = sim->sim_time;
        run_start_wall_time = (long double)microsec_timer() / 1000000.0;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_STOP:
        state = STATE_STOP;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_BACK: 
    case SDL_EVENT_QUIT:
        done = true;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_SELECT:
        state = STATE_STOP;
        curr_display = CURR_DISPLAY_SELECTION;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_SIMPANE_ZOOM_IN:
        sim_width /= 2;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_SIMPANE_ZOOM_OUT:
        sim_width *= 2;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_SIMPANE_MOUSE_CLICK: {
        int32_t found_obj = -1;
        for (i = 0; i < sim->max_object; i++) {
            object_t * p = &sim->object[i];
            int32_t disp_x, disp_y;

            disp_x = sim_pane.x + (p->X - sim_x + sim_width/2) * (sim_pane_width / sim_width);
            disp_y = sim_pane.y + (p->Y - sim_y + sim_width/2) * (sim_pane_width / sim_width);
            if (disp_x >= event->mouse_click.x - 15 &&
                disp_x <= event->mouse_click.x + 15 &&
                disp_y >= event->mouse_click.y - 15 &&
                disp_y <= event->mouse_click.y + 15 &&
                (found_obj == -1 || sim->object[i].MASS > sim->object[found_obj].MASS))
            {
                found_obj = i;
            }
        }
        if (found_obj == -1) {
            break;
        }
        tracker_obj = (found_obj == tracker_obj ? -1 : found_obj);
        sdl_play_event_sound();
        break; }
    case SDL_EVENT_SIMPANE_MOUSE_MOTION:
        sim_x -= (event->mouse_motion.delta_x * (sim_width / sim_pane_width));
        sim_y -= (event->mouse_motion.delta_y * (sim_width / sim_pane_width));
        break;
    case SDL_EVENT_TRACKERPANE_ZOOM_IN:
        tracker_width /= 2;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_TRACKERPANE_ZOOM_OUT:
        tracker_width *= 2;
        sdl_play_event_sound();
        break;
    default:
        break;
    }

    // return done flag
    return done;
}

bool sim_gravity_display_selection(bool new_display)
{
    SDL_Rect       title_line_pane;
    SDL_Rect       selection_pane;
    int32_t        title_line_pane_height, i, selection;
    sdl_event_t  * event;
    bool           done = false;

    static int32_t max_select_filenames;
    static char ** select_filenames;

    #define LIST_FILES_FREE() \
        do { \
            list_files_free(max_select_filenames, select_filenames); \
            max_select_filenames = 0; \
            select_filenames = NULL; \
        } while (0)

    //
    // if this display has just been activated then 
    // obtain list of files in the sim_gravity directory
    //

    if (new_display) {
        list_files("sim_gravity", &max_select_filenames, &select_filenames);
        INFO("XXX max_select_filenames %d\n", max_select_filenames);
    }

    //
    // short delay
    //

    usleep(1000);

    //
    // this display has 2 panes
    // - title_line_pane: the top 2 lines
    // - selection_pane: the remainder
    //

    title_line_pane_height = sdl_font[0].char_height * 2;
    SDL_INIT_PANE(title_line_pane, 
                  0, 0,  
                  sdl_win_width, title_line_pane_height);
    SDL_INIT_PANE(selection_pane, 
                  0, title_line_pane_height,
                  sdl_win_width, sdl_win_height - title_line_pane_height);
    sdl_event_init();

    //
    // clear window
    //

    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);

    //
    // render the title line and selection lines
    //

    sdl_render_text_font0(&title_line_pane, 0, 0, "SIM GRAVITY SELECTION", SDL_EVENT_NONE);
    sdl_render_text_font0(&title_line_pane, 0, SDL_PANE_COLS(&title_line_pane,0)-4, "BACK", SDL_EVENT_BACK);

    for (i = 0; i < max_select_filenames; i++) {
        sdl_render_text_font0(&selection_pane, 2*i, 0, select_filenames[i], SDL_EVENT_USER_START+i);
    }

    //
    // present the display
    //

    SDL_RenderPresent(sdl_renderer);

    //
    // handle events
    //

    event = sdl_poll_event();
    if (event->event >= SDL_EVENT_USER_START && event->event < SDL_EVENT_USER_START+max_select_filenames) {
        sdl_play_event_sound();
        selection = event->event - SDL_EVENT_USER_START;
        INFO("XXX GOT SELECTION %d\n", selection);
        LIST_FILES_FREE();
        curr_display = CURR_DISPLAY_SIMULATION;
    } else if (event->event == SDL_EVENT_BACK) {
        sdl_play_event_sound();
        LIST_FILES_FREE();
        curr_display = CURR_DISPLAY_SIMULATION;
    } else if (event->event == SDL_EVENT_QUIT) {
        sdl_play_event_sound();
        LIST_FILES_FREE();
        done = true;
    }

    // return done flag
    return done;
}

// -----------------  THREAD  ---------------------------------------------------

void * sim_gravity_thread(void * cx)
{
    int32_t thread_id = (int64_t)cx;
    int32_t local_state;

    // all threads print starting message
    INFO("thread starting, thread_id=%d\n", thread_id);

    while (true) {
        // rendezvous
        local_state = sim_gravity_rendezvoud(thread_id);

        // process based on state
        if (local_state == STATE_RUN) {
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

            int32_t i, k;
            long double SUMX, SUMY, Mi, XDISTi, YDISTi, Ri, AX, AY, DX, DY, V1X, V1Y, V2X, V2Y, tmp;
            object_t * EVALOBJ, * OTHEROBJ;

            if (thread_id == 0) {
                sim->sim_time += DT;
            }

            // XXX integer math?
            // XXX don't use long double
            for (k = thread_id; k < sim->max_object; k += max_thread) {
                EVALOBJ = &sim->object[k];

                SUMX = SUMY = 0;
                for (i = 0; i < sim->max_object; i++) {
                    if (i == k) continue;

                    OTHEROBJ = &sim->object[i];

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
                EVALOBJ->VX = DX < 0 ? -V2X : V2X;  // xxx may not be correct
                EVALOBJ->VY = DY < 0 ? -V2Y : V2Y;
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

int32_t sim_gravity_rendezvoud(int32_t thread_id)
{
    static volatile int32_t count[MAX_THREAD];
    static volatile int32_t go_count;
    static volatile int32_t ret_state;

    int32_t my_count, i;

    // just return if max_thread is 1
    if (max_thread == 1) {
        return state;
    }

    // increment count for calling thread_id
    count[thread_id]++;

    // XXX comments
    my_count = count[thread_id];

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

//asm volatile("pause\n": : :"memory");
//__builtin_ia32_pause


#if 0 //XXX delete
    object_t sun, venus, earth, moon;

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

    sun.X     = 0;
    sun.Y     = 0;
    sun.VX    = 0;
    sun.VY    = 0;
    sun.MASS  = M_SUN;
    sun.DISP_COLOR = YELLOW;

    venus.X     = R_VENUS;
    venus.Y     = 0;
    venus.VX    = 0;
    venus.VY    = TWO_PI * R_VENUS / T_VENUS;
    venus.MASS  = M_VENUS;
    venus.DISP_COLOR = WHITE;

    earth.X     = R_EARTH;
    earth.Y     = 0;
    earth.VX    = 0;
    earth.VY    = TWO_PI * R_EARTH / T_EARTH;
    earth.MASS  = M_EARTH;
    earth.DISP_COLOR = GREEN;

    moon.X    = earth.X + R_MOON;
    moon.Y    = earth.Y;
    moon.VX   = 0;
    moon.VY   = earth.VY + TWO_PI * R_MOON / T_MOON;
    moon.MASS = M_MOON;
    moon.DISP_COLOR = WHITE;

    sim->object[0] = sun;
    sim->object[1] = venus;
    sim->object[2] = earth;
    sim->object[3] = moon;

    sim->sim_time  = 0;
    sim->max_object = MAX_OBJECT;

    sim_gravity_set_obj_disp_r();
#endif

