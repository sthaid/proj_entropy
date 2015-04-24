//XXX dt rate
//XXX include doube in position_t
/**

    SOLAR_SYSTEM   

RUN              STOP

DT+              DT-

SELECT

RUN xxxx xx:xx:xx

DT  5.0 SEC

RATE  500000000:1


                   BACK
**/

// XXX display width in light minutes or seconds

// XXX arrange initial solor_system for minimal sun velocity

// XXX add more moons, on jupiter

// XXX hover on planet to diplay name

// XXX stabilize moon

// XXX select and display DT

// XXX include DT in file too, with different values for android

// XXX add other stars

// XXX reset simpane to center and also default width ??

// DONE
// - use different colors for planets
// - display track

// NOT PLANNED
// - if forces are not significant then don't inspect every time

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
// * Distance to Pluto = 
//     13 light hours
//     1.4E13 meters
//
// * example of compile time assert
//    _Static_assert(PURPLE==0, "purple");
//

//
// defines
//

#define TWO_PI     (2.0*M_PI)
#define G          6.673E-11       // mks units  
#define AU         1.495978707E11  // meters

#define MAX_OBJECT  1000
#define MAX_THREAD  16
#define MAX_NAME    32

#define SDL_EVENT_STOP                   (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN                    (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_DT_PLUS                (SDL_EVENT_USER_START + 2)
#define SDL_EVENT_DT_MINUS               (SDL_EVENT_USER_START + 3)
#define SDL_EVENT_SELECT                 (SDL_EVENT_USER_START + 4)
#define SDL_EVENT_SIMPANE_ZOOM_IN        (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_SIMPANE_ZOOM_OUT       (SDL_EVENT_USER_START + 11)
#define SDL_EVENT_SIMPANE_MOUSE_CLICK    (SDL_EVENT_USER_START + 12)
#define SDL_EVENT_SIMPANE_MOUSE_MOTION   (SDL_EVENT_USER_START + 13)
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

#define DEFAULT_SIM_GRAVITY_FILENAME "solar_system"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define ZOOM_FACTOR (exp(log(10)/5))
#define SIM_WIDTH_INITIAL (AU * ZOOM_FACTOR * ZOOM_FACTOR)

//
// typedefs
//

// XXX range for X,Y,SIM_WIDTH

typedef struct {
    int64_t METERS;
    int64_t NANOMETERS;
} position_t;

typedef struct {
    char        NAME[MAX_NAME];
    position_t  X;             // meters
    position_t  Y;
    double      VX;            // meters/secod
    double      VY;
    double      MASS;          // kg
    int64_t     RADIUS;        // meters

    position_t  NEXT_X; 
    position_t  NEXT_Y;
    double      NEXT_VX; 
    double      NEXT_VY;

    int32_t DISP_COLOR;
    int32_t DISP_R; 

    int32_t MAX_PATH;
    int32_t PATH_TAIL;
    struct path_s {
        int64_t X_METERS;
        int64_t Y_METERS;
    } PATH[0];
} object_t; 

typedef struct {
    char       sim_name[MAX_NAME];
    int64_t    sim_time;
    int32_t    max_object;
    object_t * object[MAX_OBJECT];
} sim_t;

//
// variables
//

static sim_t        sim;

static int32_t      state; 
static int64_t      sim_width;  //xxx searhch  XXX check for max
static int64_t      sim_x;  //xxx searhch
static int64_t      sim_y;  //xxx searhch
static int32_t      DT;
static int32_t      tracker_obj;

static int64_t      run_start_sim_time;
static int64_t      run_start_wall_time;

static int32_t      max_thread;
static pthread_t    thread_id[MAX_THREAD];

static int32_t      curr_display;

// 
// prototypes
//

int32_t sim_gravity_init(void);
void sim_gravity_terminate(void);
int32_t sim_gravity_init_sim_from_file(char * filename);
void sim_gravity_set_obj_disp_r(object_t ** obj, int32_t max_object);
bool sim_gravity_display(void);
bool sim_gravity_display_simulation(bool new_display);
bool sim_gravity_display_selection(bool new_display);
void * sim_gravity_thread(void * cx);
int32_t sim_gravity_barrier(int32_t thread_id);
position_t sim_gravity_position_add(position_t x, double v);

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
    int32_t i;

    // print info msg
    INFO("initialize start\n");

    // xxx tbd
    state = STATE_STOP; 
    sim_width = SIM_WIDTH_INITIAL;
    sim_x = 0;
    sim_y = 0;
    tracker_obj = -1;

    strncpy(sim.sim_name, "GRAVITY", sizeof(sim.sim_name)-1);
    DT = 5;

    // init sim  
    sim_gravity_init_sim_from_file(DEFAULT_SIM_GRAVITY_FILENAME);

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
    INFO("max_thread=%d\n", max_thread);
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
    for (i = 0; i < sim.max_object; i++) {
        free(sim.object[i]);
    }
    bzero(&sim, sizeof(sim));

    // done
    INFO("terminate complete\n");
}

// -----------------  SIM GRAVITY INIT  -----------------------------------------

int32_t sim_gravity_init_sim_from_file(char * filename) 
{
    #define MAX_BUFF  1000000
    #define MAX_RELTO 100

    #define OBJ_ALLOC_SIZE(max_path) (sizeof(object_t) + (max_path) * sizeof(struct path_s))

    typedef struct {
        double X;
        double Y;
        double VX;
        double VY;
    } relto_t;

    object_t  * object[MAX_OBJECT];

    char        pathname[200];
    int32_t     len, i, cnt;
    char      * s, * s_next, * tmp;
    object_t  * obj;
    relto_t     relto[MAX_RELTO];
    double      X, Y, VX, VY, MASS, RADIUS;
    char        NAME[100], COLOR[100];
    int32_t     MAX_PATH;

    SDL_RWops * rw = NULL;
    char      * buff = NULL;
    int32_t     ret = 0;
    int32_t     line = 0;
    int32_t     idx = 0;
    int32_t     last_idx = 0;
    int32_t     max_object = 0;

    bzero(object, sizeof(object));
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
        cnt = sscanf(s, "%s %lf %lf %lf %lf %lf %lf %s %d",
                     NAME, &X, &Y, &VX, &VY, &MASS, &RADIUS, COLOR, &MAX_PATH);
        if (cnt != 9) {
            ERROR("line %d invalid in file %s\n", line, filename);
            ret = -1;
            goto done;
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

        // initialize obj, note DISP_R field is initialized by call to sim_gravity_set_obj_disp_r below
        // xxx check alloc
        object_t * obj = calloc(1,OBJ_ALLOC_SIZE(MAX_PATH));
        strncpy(obj->NAME, NAME, MAX_NAME-1);
        obj->X.METERS   = X + relto[idx-1].X;
        obj->Y.METERS   = Y + relto[idx-1].Y;
        obj->VX         = VX + relto[idx-1].VX;
        obj->VY         = VY + relto[idx-1].VY;
        obj->MASS       = MASS;
        obj->RADIUS     = RADIUS;
        obj->DISP_COLOR = COLOR_STR_TO_COLOR(COLOR);
        obj->MAX_PATH   = MAX_PATH;

        // save new relto
        relto[idx].X  = obj->X.METERS;
        relto[idx].Y  = obj->Y.METERS;
        relto[idx].VX = obj->VX;
        relto[idx].VY = obj->VY;

        // save the new obj in object array
        object[max_object++] = obj;
    }

    // fill in DISP_R fields
    sim_gravity_set_obj_disp_r(object, max_object);

    // success ...
    // xxx stop all threads when this is being run

    // free existing sim objects 
    for (i = 0; i < sim.max_object; i++) {
        free(sim.object[i]);
    }
    bzero(&sim,sizeof(sim));

    // incorporate new objects into sim
    strncpy(sim.sim_name, filename, sizeof(sim.sim_name)-1);
    for (i = 0; sim.sim_name[i]; i++) {
        sim.sim_name[i] = toupper(sim.sim_name[i]);
    }
    sim.sim_time = 0;
    sim.max_object = max_object;
    for (i = 0; i < sim.max_object; i++) {
        sim.object[i] = object[i];
        object[i] = NULL;
    }

    // xxx tbd, init from file
    state = STATE_STOP; 
    sim_width = SIM_WIDTH_INITIAL;
    sim_x = 2.88E16; //XXX
    sim_y = 2.88E16;
    tracker_obj = -1;

    ret = 0;

#if 1
    // print simulation config
    PRINTF("------ FILENAME %s ------\n", filename);   
    PRINTF("NAME                    X            Y        X_VEL        Y_VEL         MASS       RADIUS        COLOR       DISP_R     MAX_PATH\n");
         // ------------ ------------ ------------ ------------ ------------ ------------ ------------ ------------ ------------ ------------
    for (i = 0; i < sim.max_object; i++) {
        obj = sim.object[i];
        PRINTF("%-12s %12lg %12lg %12lg %12lg %12lg %12lg %12d %12d %12d\n",
            obj->NAME, (double)obj->X.METERS, (double)obj->Y.METERS, obj->VX, obj->VY, 
            obj->MASS, (double)obj->RADIUS, obj->DISP_COLOR, obj->DISP_R, obj->MAX_PATH);
    }
    // xxx also print other settings
    PRINTF("-------------------------\n");   
#endif

done:
    // cleanup and return
    if (rw != NULL) {
        SDL_RWclose(rw);
    }
    for (i = 0; i < MAX_OBJECT; i++) {
        free(object[i]);
    }
    free(buff);

    return ret;
}

void sim_gravity_set_obj_disp_r(object_t ** obj, int32_t max_object)
{
    double radius[max_object]; 
    double min_radius, max_radius, min_disp_r, max_disp_r, delta;
    int32_t i;

    #define MIN_DISP_R (2.)
    #define MAX_DISP_R (5. * MIN_DISP_R)
    #define CENTER_DISP_R ((MAX_DISP_R + MIN_DISP_R) / 2.)

    // fill in radius raw values with cube root of mass
    for (i = 0; i < max_object; i++) {
        radius[i] = obj[i]->RADIUS;
    }

    // determine min and max raw radius values
    min_radius = 1E99;
    max_radius = -1E99;
    for (i = 0; i < max_object; i++) {
        if (radius[i] < min_radius) {
            min_radius = radius[i];
        }
        if (radius[i] > max_radius) {
            max_radius = radius[i];
        }
    }

    // if min and max equal then
    //   set obj_radius to CENTER_DISP_R
    if (min_radius == max_radius) {
        for (i = 0; i < max_object; i++) {
            obj[i]->DISP_R = CENTER_DISP_R;
        }

    // else if max / min < 5 then
    //   map raw radius values to range min_disp_r to max_disp_r,
    //   where these values have ratio equal to max_radius/min_radius
    } else if (max_radius / min_radius <= 5) {
        delta = (max_radius / min_radius * CENTER_DISP_R - CENTER_DISP_R) / 
                (max_radius  / min_radius + 1);
        min_disp_r = CENTER_DISP_R - delta;
        max_disp_r = CENTER_DISP_R + delta;

        for (i = 0; i < max_object; i++) {
            obj[i]->DISP_R = (max_disp_r - min_disp_r) / (max_radius - min_radius) * 
                             (radius[i] - min_radius ) + min_disp_r;
        }

    // else 
    //   convert raw radius values to log scale
    //   map these new raw radius values to MIN_DISP_R to MAX_DISP_R
    } else {
        for (i = 0; i < max_object; i++) {
            radius[i] = log(radius[i]);
        }

        min_radius = 1E99;
        max_radius = -1E99;
        for (i = 0; i < max_object; i++) {
            if (radius[i] < min_radius) {
                min_radius = radius[i];
            }
            if (radius[i] > max_radius) {
                max_radius = radius[i];
            }
        }

        for (i = 0; i < max_object; i++) {
            obj[i]->DISP_R = (MAX_DISP_R - MIN_DISP_R) / (max_radius - min_radius) * 
                             (radius[i] - min_radius) + MIN_DISP_R;
        }
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

    SDL_Rect       ctl_pane, sim_pane;
    int32_t        i, col;
    double         sim_pane_width;
    sdl_event_t *  event;
    char           str[100], str1[100];
    bool           done = false;

    static SDL_Texture * circle_texture_tbl[MAX_CIRCLE_RADIUS][MAX_COLOR];

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
    // - ctl_pane: the controls display area
    //

    if (sdl_win_width > sdl_win_height) {
        sim_pane_width = sdl_win_height;
        SDL_INIT_PANE(sim_pane, 
                      0, 0,  
                      sim_pane_width, sim_pane_width);
        SDL_INIT_PANE(ctl_pane, 
                      sim_pane_width, 0, 
                      sdl_win_width-sim_pane_width, sdl_win_height);
    } else {
        sim_pane_width = sdl_win_width;
        SDL_INIT_PANE(sim_pane, 0, 0,  sim_pane_width, sim_pane_width);
        SDL_INIT_PANE(ctl_pane, 0, 0, 0, 0);
    }
    sdl_event_init();

    //
    // short delay
    //

    usleep(5000);

    //
    // clear window
    //

    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);

    //
    // SIMPANE ...
    //

    // draw sim_pane objects 
    for (i = 0; i < sim.max_object; i++) {
        object_t * obj = sim.object[i];
        object_t * obj_tracker;
        int32_t    disp_x, disp_y, disp_r, disp_color;
        int32_t    obj_idx, tracker_idx, idx_step, count, total, max_path;
        int64_t    X, Y, X_ORIGIN, Y_ORIGIN;
        SDL_Point  points[1000]; //xxx

        // display the object ...

        // determine display origin
        if (tracker_obj == -1) {
            X_ORIGIN = sim_x;
            Y_ORIGIN = sim_y;
        } else {
            X_ORIGIN = sim.object[tracker_obj]->X.METERS;
            Y_ORIGIN = sim.object[tracker_obj]->Y.METERS;
        }

        // determine object's display info
        // - position: disp_x, disp_y
        // - radius: disp_r
        // - color: disp_color
        disp_x = sim_pane.x + (obj->X.METERS - X_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
        disp_y = sim_pane.y + (obj->Y.METERS - Y_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
        disp_r = (obj->DISP_R < 1 ? 1 : obj->DISP_R >= MAX_CIRCLE_RADIUS ? MAX_CIRCLE_RADIUS-1 : obj->DISP_R);
        disp_color = (i != tracker_obj ? obj->DISP_COLOR : PINK);

        // if circle to represent this object has not yet been created then create it
        if (circle_texture_tbl[disp_r][disp_color] == NULL) {
            circle_texture_tbl[disp_r][disp_color] =
                sdl_create_filled_circle_texture(disp_r, sdl_pixel_rgba[disp_color]);
        }

        // render the circle, representing the object
        sdl_render_circle(disp_x, disp_y, circle_texture_tbl[disp_r][disp_color]);

        // display object's path ...
        // xxx clean up and comment

        if (tracker_obj == -1) {
            obj_tracker = NULL;
            max_path = obj->MAX_PATH;
            obj_idx = (obj->PATH_TAIL == 0 ? obj->MAX_PATH-1 : obj->PATH_TAIL-1);
            tracker_idx = 0;
        } else {
            obj_tracker = sim.object[tracker_obj];
            max_path = MIN(obj->MAX_PATH, obj_tracker->MAX_PATH);
            if (max_path > 50) {
                max_path = 50;
            }
            obj_idx = (obj->PATH_TAIL == 0 ? obj->MAX_PATH-1 : obj->PATH_TAIL-1);
            tracker_idx = (obj_tracker->PATH_TAIL == 0 ? obj_tracker->MAX_PATH-1 : obj_tracker->PATH_TAIL-1);
        }

        if (obj == obj_tracker) {
            continue;
        }

        idx_step = (max_path/100 >= 1 ? max_path/100 : 1);
        total = max_path / idx_step;
        count = 0;

        while (true) {
            if (obj_tracker == NULL) {
                X_ORIGIN = sim_x;
                Y_ORIGIN = sim_y;
            } else {
                X_ORIGIN = obj_tracker->PATH[tracker_idx].X_METERS;
                Y_ORIGIN = obj_tracker->PATH[tracker_idx].Y_METERS;
                if (X_ORIGIN == 0 && Y_ORIGIN == 0) {
                    break;
                }
            }

            X = obj->PATH[obj_idx].X_METERS;
            Y = obj->PATH[obj_idx].Y_METERS;
            if (X == 0 && Y == 0) {
                break;
            }

            points[count].x = sim_pane.x + (X - X_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
            points[count].y = sim_pane.y + (Y - Y_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
            count++;

            if (count == total) {
                break;
            }

            obj_idx = (obj_idx-idx_step < 0 ? obj_idx-idx_step+obj->MAX_PATH : obj_idx-idx_step);
            if (obj_tracker != NULL) {
                tracker_idx = (tracker_idx-idx_step < 0 ? tracker_idx-idx_step+obj_tracker->MAX_PATH : tracker_idx-idx_step);
            }
        }
        SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255); //XXX
        SDL_RenderDrawLines(sdl_renderer, points, count);
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
    // CTLPANE ...
    //

    // clear ctl_pane
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(sdl_renderer, &ctl_pane);

    // display controls  
    sdl_render_text_ex(&ctl_pane, 0, 0, sim.sim_name, SDL_EVENT_NONE,
                       SDL_FIELD_COLS_UNLIMITTED, true, 0);
    sdl_render_text_font0(&ctl_pane,  2, 0,  "RUN",     SDL_EVENT_RUN);
    sdl_render_text_font0(&ctl_pane,  2, 9,  "STOP",    SDL_EVENT_STOP);
    sdl_render_text_font0(&ctl_pane,  4, 0,  "DT+",     SDL_EVENT_DT_PLUS);
    sdl_render_text_font0(&ctl_pane,  4, 9,  "DT-",     SDL_EVENT_DT_MINUS);
    sdl_render_text_font0(&ctl_pane,  6, 0,  "SELECT",  SDL_EVENT_SELECT);
    sdl_render_text_font0(&ctl_pane, 18,15,  "BACK",    SDL_EVENT_BACK);

    // display status lines
    sprintf(str, "%-4s %s", STATE_STR(state), dur2str(str1,sim.sim_time));
    sdl_render_text_font0(&ctl_pane, 8, 0, str, SDL_EVENT_NONE);

    sprintf(str, "DT   %d SECS", DT);
    sdl_render_text_font0(&ctl_pane, 10, 0, str, SDL_EVENT_NONE);

    if (state == STATE_RUN) {
        int32_t perf = (sim.sim_time - run_start_sim_time) / (microsec_timer()/1000000.0 - run_start_wall_time);
        sprintf(str, "RATE %d:1", perf);
        sdl_render_text_font0(&ctl_pane, 12, 0, str, SDL_EVENT_NONE);
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
        run_start_sim_time = sim.sim_time;
        run_start_wall_time = microsec_timer() / 1000000.0;
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
    case SDL_EVENT_DT_PLUS:
        DT += 1;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_DT_MINUS:
        if (DT > 1.1) {
            DT -= 1;
        }
        sdl_play_event_sound();
        break;
    case SDL_EVENT_SELECT:
        state = STATE_STOP;
        curr_display = CURR_DISPLAY_SELECTION;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_SIMPANE_ZOOM_IN:
        sim_width /= ZOOM_FACTOR;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_SIMPANE_ZOOM_OUT:
        sim_width *= ZOOM_FACTOR;
        sdl_play_event_sound();
        break;
    case SDL_EVENT_SIMPANE_MOUSE_CLICK: {
        int32_t found_obj = -1;
        for (i = 0; i < sim.max_object; i++) {
            object_t * obj = sim.object[i];
            int32_t disp_x, disp_y;
            int64_t X_ORIGIN, Y_ORIGIN;

            if (tracker_obj == -1) {
                X_ORIGIN = sim_x;
                Y_ORIGIN = sim_y;
            } else {
                X_ORIGIN = sim.object[tracker_obj]->X.METERS;
                Y_ORIGIN = sim.object[tracker_obj]->Y.METERS;
            }

            disp_x = sim_pane.x + (obj->X.METERS - X_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
            disp_y = sim_pane.y + (obj->Y.METERS - Y_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);

            if (disp_x >= event->mouse_click.x - 15 &&
                disp_x <= event->mouse_click.x + 15 &&
                disp_y >= event->mouse_click.y - 15 &&
                disp_y <= event->mouse_click.y + 15 &&
                (found_obj == -1 || sim.object[i]->MASS > sim.object[found_obj]->MASS))
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
        if (tracker_obj == -1) {
            sim_x -= (event->mouse_motion.delta_x * (sim_width / sim_pane_width));
            sim_y -= (event->mouse_motion.delta_y * (sim_width / sim_pane_width));
        }
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
    }

    //
    // short delay
    //

    usleep(5000);

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
        INFO("xxx GOT SELECTION %d\n", selection);
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
        local_state = sim_gravity_barrier(thread_id);

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
            double SUMX, SUMY, Mi, XDISTi, YDISTi, Ri, AX, AY, DX, DY, V1X, V1Y, V2X, V2Y, tmp;
            object_t * EVALOBJ, * OTHEROBJ;

            for (k = thread_id; k < sim.max_object; k += max_thread) {
                EVALOBJ = sim.object[k];

                SUMX = SUMY = 0;
                for (i = 0; i < sim.max_object; i++) {
                    if (i == k) continue;

                    OTHEROBJ = sim.object[i];

                    Mi      = OTHEROBJ->MASS;
                    XDISTi  = (double)OTHEROBJ->X.METERS - (double)EVALOBJ->X.METERS;
                    YDISTi  = (double)OTHEROBJ->Y.METERS - (double)EVALOBJ->Y.METERS;
                    Ri      = sqrt(XDISTi * XDISTi + YDISTi * YDISTi);
                    if (Ri == 0) {
                        continue;
                    }
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
                V2X = V1X + AX * DT;
                V2Y = V1Y + AY * DT;

#if 1
                EVALOBJ->NEXT_X = sim_gravity_position_add(EVALOBJ->X, DX);
                EVALOBJ->NEXT_Y = sim_gravity_position_add(EVALOBJ->Y, DY);
#else
                EVALOBJ->NEXT_X  = EVALOBJ->X + DX;
                EVALOBJ->NEXT_Y  = EVALOBJ->Y + DY;
#endif
                EVALOBJ->NEXT_VX = V2X;
                EVALOBJ->NEXT_VY = V2Y;
            }

            sim_gravity_barrier(thread_id);

            if (thread_id == 0) {
                int32_t day = sim.sim_time/86400;
                static int32_t last_day;
                for (k = 0; k < sim.max_object; k++) {
                    object_t * obj = sim.object[k];
                    obj->X  = obj->NEXT_X;
                    obj->Y  = obj->NEXT_Y;
                    obj->VX = obj->NEXT_VX;
                    obj->VY = obj->NEXT_VY;

                    // XXX more work here
                    if (day != last_day) {
                        int32_t idx = obj->PATH_TAIL;
                        obj->PATH[idx].X_METERS = obj->X.METERS;
                        obj->PATH[idx].Y_METERS = obj->Y.METERS;
                        asm volatile("" ::: "memory");
                        obj->PATH_TAIL = (idx == obj->MAX_PATH - 1 ? 0 : idx + 1);
                    }
                }
                last_day = day;

                sim.sim_time += DT;
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

int32_t sim_gravity_barrier(int32_t thread_id)
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

position_t sim_gravity_position_add(position_t x, double v)
{
    int64_t meters, nanometers;
    position_t result;

    if (v == 0) {
        return x;
    }

    if (v > 0) {
        meters = (int64_t)floor(v);
        nanometers = (v - meters) * 1000000000;

        result.NANOMETERS = x.NANOMETERS + nanometers;
        result.METERS     = x.METERS + meters;
        if (result.NANOMETERS >= 1000000000) {
            result.NANOMETERS -= 1000000000;
            result.METERS++;
        }
    } else {
        v = -v;
        meters = (int64_t)floor(v);
        nanometers = (v - meters) * 1000000000;

        result.NANOMETERS = x.NANOMETERS - nanometers;
        result.METERS     = x.METERS - meters;
        if (result.NANOMETERS < 0) {
            result.NANOMETERS += 1000000000;
            result.METERS--;
        }
    }

    return result;
}
