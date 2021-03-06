#include "util.h"
#include "sim_gravity_help.h"

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

#define G   6.673E-11       // mks units  
#define AU  1.495978707E11  // meters
#define LY  9.4605284E15    // meters

#define DEFAULT_SIM_GRAVITY_PATHNAME "sim_gravity/solar_system"

#define MAX_OBJECT          200
#define MAX_THREAD          16
#define MAX_NAME            32
#define MAX_PATH            (260 * 365 + 260 / 4)
#define MIN_PATH_DISP_DAYS  (365.25 / 8)
#define MAX_PATH_DISP_DAYS  (256 * 365.25)

#define SDL_EVENT_STOP                     (SDL_EVENT_USER_START + 0)
#define SDL_EVENT_RUN                      (SDL_EVENT_USER_START + 1)
#define SDL_EVENT_DT_PLUS                  (SDL_EVENT_USER_START + 2)
#define SDL_EVENT_DT_MINUS                 (SDL_EVENT_USER_START + 3)
#define SDL_EVENT_SELECT                   (SDL_EVENT_USER_START + 4)
#define SDL_EVENT_PATH_DISP_DEFAULT        (SDL_EVENT_USER_START + 6)
#define SDL_EVENT_PATH_DISP_OFF            (SDL_EVENT_USER_START + 7)
#define SDL_EVENT_PATH_DISP_PLUS           (SDL_EVENT_USER_START + 8)
#define SDL_EVENT_PATH_DISP_MINUS          (SDL_EVENT_USER_START + 9)
#define SDL_EVENT_HELP                     (SDL_EVENT_USER_START + 10)
#define SDL_EVENT_BACK                     (SDL_EVENT_USER_START + 11)
#define SDL_EVENT_SIMPANE_ZOOM_IN          (SDL_EVENT_USER_START + 20)
#define SDL_EVENT_SIMPANE_ZOOM_OUT         (SDL_EVENT_USER_START + 21)
#define SDL_EVENT_SIMPANE_MOUSE_MOTION     (SDL_EVENT_USER_START + 22)
#define SDL_EVENT_SIMPANE_MOUSE_CLICK      (SDL_EVENT_USER_START + 23)  // ... SDL_EVENT_USER_END
#define SDL_EVENT_SIMPANE_MOUSE_CLICK_LAST (SDL_EVENT_USER_END)

#define STATE_STOP               0
#define STATE_RUN                1
#define STATE_TERMINATE_THREADS  9

#define STATE_STR(s) \
    ((s) == STATE_STOP              ? "STOP"              : \
     (s) == STATE_RUN               ? "RUN"               : \
     (s) == STATE_TERMINATE_THREADS ? "TERMINATE_THREADS"   \
                                    : "????")
#define DISPLAY_TERMINATE    1
#define DISPLAY_SIMULATION   2
#define DISPLAY_SELECT       3
#define DISPLAY_HELP         5
#define DISPLAY_ERROR        6

#define MAX_VALID_DT (sizeof(valid_dt)/sizeof(valid_dt[0]))
#define MAX_VALID_SIM_WIDTH (sizeof(valid_sim_width)/sizeof(valid_sim_width[0]))

#define IS_CLOSE(a,b) ((a) >= 0.999*(b) && (a) <= 1.001*(b))

//
// typedefs
//

typedef struct {
    char        NAME[MAX_NAME];
    double      X;             // meters
    double      Y;
    double      VX;            // meters/secod
    double      VY;
    double      MASS;          // kg
    double      RADIUS;        // meters

    double      NEXT_X; 
    double      NEXT_Y;
    double      NEXT_VX; 
    double      NEXT_VY;

    int32_t DISP_COLOR;
    int32_t DISP_R; 

    int32_t MAX_PATH_DISP_DFLT; 
    struct path_s {
        double X;
        double Y;
    } PATH[MAX_PATH];
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
static double       sim_width;
static double       sim_x;
static double       sim_y;
static int32_t      DT;
static int32_t      tracker_obj;

static int64_t      perf_start_sim_time;
static int64_t      perf_start_wall_time;

static int32_t      max_thread;
static pthread_t    thread_id[MAX_THREAD];

static double       path_disp_days = -1;
static volatile int64_t path_tail;

static char          error_str[3][200];

static const int32_t valid_dt[] = {
                        1,2,3,4,5,6,7,8,9,
                        10,20,30,40,50,
                        60,120,180,240,300,360,420,480,540,
                        600,1200,1800,2400,3000,
                        3600,2*3600,3*3600,4*3600,5*3600,6*3600,
                        86400, };

static const double valid_sim_width[] = {
#if 0
                        .01*AU, .03*AU,
                        .1*AU, .3*AU,
                        1*AU, 3*AU,
                        10*AU, 30*AU,
                        100*AU, 300*AU,
                        1000*AU, 3000*AU,
                        10000*AU, 30000*AU,
                        100000*AU, 300000*AU,
                        1000000*AU, 3000000*AU, };
#endif
                            .01*AU,     .02*AU,     .05*AU, 
                             .1*AU,      .2*AU,      .5*AU, 
                              1*AU,       2*AU,       5*AU, 
                             10*AU,      20*AU,      50*AU, 
                            100*AU,     200*AU,     500*AU, 
                           1000*AU,    2000*AU,    5000*AU, 
                          10000*AU,   20000*AU,   50000*AU, 
                         100000*AU,  200000*AU,  500000*AU, 
                        1000000*AU, 2000000*AU, 5000000*AU, 
                                };

// 
// prototypes
//

int32_t sim_gravity_init(char * pathname);
void sim_gravity_terminate(void);
void sim_gravity_set_obj_disp_r(object_t ** obj, int32_t max_object);
void sim_gravity_display(void);
int32_t sim_gravity_display_simulation(int32_t curr_display, int32_t last_display);
int32_t sim_gravity_display_select(int32_t curr_display, int32_t last_display);
int32_t sim_gravity_display_help(int32_t curr_display, int32_t last_display);
int32_t sim_gravity_display_error(int32_t curr_display, int32_t last_display);
void * sim_gravity_thread(void * cx);
int32_t sim_gravity_barrier(int32_t thread_id);

// -----------------  MAIN  -----------------------------------------------------

void sim_gravity(void) 
{
    sim_gravity_init(DEFAULT_SIM_GRAVITY_PATHNAME);

    sim_gravity_display();

    sim_gravity_terminate();
}

// -----------------  INIT & TERMINATE  -----------------------------------------

int32_t sim_gravity_init(char * pathname)
{
    #define MAX_RELTO 100

    typedef struct {
        double X;
        double Y;
        double VX;
        double VY;
    } relto_t;

    object_t  * object[MAX_OBJECT]; 

    int32_t     i, cnt;
    char      * s;
    relto_t     relto[MAX_RELTO];
    char        shortname[200];
    double      X, Y, VX, VY, MASS, RADIUS;
    char        NAME[MAX_NAME], COLOR[100];
    int32_t     MAX_PATH_DISP_DFLT;

    int32_t     ret           = 0;
    int32_t     line          = 0;
    int32_t     idx           = 0;
    int32_t     last_idx      = 0;
    int32_t     max_object    = 0;
    double      new_sim_width = sim_width;
    int32_t     new_dt        = DT;
    file_t    * file          = NULL;

    bzero(object, sizeof(object));
    bzero(relto, sizeof(relto));

    // print info msg
    INFO("initialize start, pathname %s\n", pathname);

    // clear error strings
    error_str[0][0] = '\0';
    error_str[1][0] = '\0';
    error_str[2][0] = '\0';

    // terminate threads
    state = STATE_TERMINATE_THREADS;
    for (i = 0; i < max_thread; i++) {
        if (thread_id[i]) {
            pthread_join(thread_id[i], NULL);
            thread_id[i] = 0;
        }
    }
    max_thread = 0;

    // reset variables  
    for (i = 0; i < sim.max_object; i++) {
        free(sim.object[i]);
    }
    bzero(&sim, sizeof(sim));
    strncpy(sim.sim_name, "GRAVITY", sizeof(sim.sim_name)-1);
    state       = STATE_STOP; 
    tracker_obj    = -1;
    sim_x          = 0;
    sim_y          = 0;
    sim_width      = 3*AU;
    DT             = 1;
    path_disp_days = -1;

    // open pathname
    file = open_file(pathname);
    if (file == NULL) {
        sprintf(error_str[0], "Open Failed");
        sprintf(error_str[1], "%s", pathname);
        ret = -1;
        goto done;
    }
        
    // process the file
    while ((s = read_file_line(file)) != NULL) {
        // increment line number
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

        // if line begins with 'PARAM' then it is a parameter line
        if (strncmp(s, "PARAM ", 6) == 0) {
            char param_name[100], param_units[100];
            double param_value;

            if (sscanf(s+6, "%s %lf %s", param_name, &param_value, param_units) != 3) {
                sprintf(error_str[0], "Invalid Parameter");
                sprintf(error_str[1], "%s", pathname);
                sprintf(error_str[2], "line %d", line);
                ret = -1;
                goto done;
            }
            if ((strcmp(param_name, "SIM_WIDTH") == 0) && 
                (strcmp(param_units, "AU") == 0 || strcmp(param_units, "LY") == 0))
            {
                new_sim_width = param_value * (strcmp(param_units, "AU")==0 ? AU : LY);
                for (i = MAX_VALID_SIM_WIDTH-1; i >= 0; i--) {
                    if (new_sim_width >= valid_sim_width[i]) {
                        new_sim_width = valid_sim_width[i];
                        break;
                    }
                }
                if (i == -1) {
                    new_sim_width = valid_sim_width[0];
                }
            } else if ((strcmp(param_name, "DT_LINUX") == 0 || strcmp(param_name, "DT_ANDROID") == 0) &&
                       (strcmp(param_units, "SEC") == 0)) 
            {
#ifdef ANDROID
                if (strcmp(param_name, "DT_LINUX") == 0) continue;
#else
                if (strcmp(param_name, "DT_ANDROID") == 0) continue;
#endif
                new_dt = param_value;
                for (i = MAX_VALID_DT-1; i >= 0; i--) {
                    if (new_dt >= valid_dt[i]) {
                        new_dt = valid_dt[i];
                        break;
                    }
                }
                if (i == -1) {
                    new_dt = valid_dt[0];
                }
            } else {
                sprintf(error_str[0], "Invalid Parameter");
                sprintf(error_str[1], "%s", pathname);
                sprintf(error_str[2], "line %d", line);
                ret = -1;
                goto done;
            }
            continue;
        }

        // scan this line
        cnt = sscanf(s, "%s %lf %lf %lf %lf %lf %lf %s %d",
                     NAME, &X, &Y, &VX, &VY, &MASS, &RADIUS, COLOR, &MAX_PATH_DISP_DFLT);
        if (cnt != 9) {
            sprintf(error_str[0], "Invalid Object");
            sprintf(error_str[1], "%s", pathname);
            sprintf(error_str[2], "line %d", line);
            ret = -1;
            goto done;
        }

        // check MAX_PATH_DISP_DFLT is in range
        if (MAX_PATH_DISP_DFLT < 0) {
            MAX_PATH_DISP_DFLT = 0;
        } else if (MAX_PATH_DISP_DFLT > MAX_PATH_DISP_DAYS) {
            MAX_PATH_DISP_DFLT = MAX_PATH_DISP_DAYS;
        }

        // determine idx for accessing relative_to[]
        for (idx = 0; s[idx] == ' '; idx++) ;
        idx++;
        if (idx > last_idx+1) {
            sprintf(error_str[0], "Invalid Indent");
            sprintf(error_str[1], "%s", pathname);
            sprintf(error_str[2], "line %d", line);
            ret = -1;
            goto done;
        }
        last_idx = idx;

        // initialize obj, note DISP_R field is initialized by call to sim_gravity_set_obj_disp_r below
        object_t * obj = calloc(1,sizeof(object_t));
        if (obj == NULL) {
            sprintf(error_str[0], "Out Of Memory");
            sprintf(error_str[1], "%s", pathname);
            sprintf(error_str[2], "line %d", line);
            ret = -1;
            goto done;
        }
        strncpy(obj->NAME, NAME, MAX_NAME);
        obj->X                  = X + relto[idx-1].X;
        obj->Y                  = Y + relto[idx-1].Y;
        obj->VX                 = VX + relto[idx-1].VX;
        obj->VY                 = VY + relto[idx-1].VY;
        obj->MASS               = MASS;
        obj->RADIUS             = RADIUS;
        obj->DISP_COLOR         = COLOR_STR_TO_COLOR(COLOR);
        obj->MAX_PATH_DISP_DFLT = MAX_PATH_DISP_DFLT;

        // save new relto
        relto[idx].X  = obj->X;
        relto[idx].Y  = obj->Y;
        relto[idx].VX = obj->VX;
        relto[idx].VY = obj->VY;

        // save the new obj in object array
        object[max_object++] = obj;

        // if used MAX_OBJECT then break
        if (max_object == MAX_OBJECT) {
            break;
        }
    }

    // fill in DISP_R fields
    sim_gravity_set_obj_disp_r(object, max_object);

    // success ...

    // init sim
    strcpy(shortname, pathname);
    strncpy(sim.sim_name, basename(shortname), sizeof(sim.sim_name)-1);
    for (i = 0; sim.sim_name[i]; i++) {
        sim.sim_name[i] = toupper(sim.sim_name[i]);
    }
    sim.sim_time = 0;
    sim.max_object = max_object;
    for (i = 0; i < sim.max_object; i++) {
        sim.object[i] = object[i];
        object[i] = NULL;
    }
    sim_width   = new_sim_width;
    DT          = new_dt;

    // create worker threads
#ifndef ANDROID
    if (sim.max_object >= 10) {
        max_thread = sysconf(_SC_NPROCESSORS_ONLN) - 1;
        if (max_thread == 0) {
            max_thread = 1;
        } else if (max_thread > MAX_THREAD) {
            max_thread = MAX_THREAD;
        }
    } else {
        max_thread = 1;
    }
#else
    max_thread = 1;
#endif
    INFO("max_thread=%d\n", max_thread);
    for (i = 0; i < max_thread; i++) {
        pthread_create(&thread_id[i], NULL, sim_gravity_thread, (void*)(long)i);
    }

#ifdef ENABLE_LOGGING
    // print simulation config
    PRINTF("------ PATHNAME %s ------\n", pathname);   
    PRINTF("                                                                                                                        MAX_PATH_\n");
    PRINTF("NAME                    X            Y        X_VEL        Y_VEL         MASS       RADIUS        COLOR       DISP_R    DISP_DFLT\n");
         // ------------ ------------ ------------ ------------ ------------ ------------ ------------ ------------ ------------ ------------
    for (i = 0; i < sim.max_object; i++) {
        object_t * obj = sim.object[i];
        PRINTF("%-12s %12lg %12lg %12lg %12lg %12lg %12lg %12d %12d %12d\n",
            obj->NAME, 
            obj->X,
            obj->Y,
            obj->VX, 
            obj->VY, 
            obj->MASS, 
            obj->RADIUS, 
            obj->DISP_COLOR, 
            obj->DISP_R, 
            obj->MAX_PATH_DISP_DFLT);
    }
    PRINTF("SIM_WIDTH %lf AU\n", sim_width/AU);
    PRINTF("DT        %d  SEC\n", DT);
    PRINTF("-------------------------\n");   
#endif

done:
    // cleanup and return
    if (file != NULL) {
        close_file(file);
    }
    for (i = 0; i < MAX_OBJECT; i++) {
        free(object[i]);
    }

    // success
    if (ret == 0) {
        INFO("initialize successful\n");
    } else {
        ERROR("%s '%s' '%s'\n", error_str[0], error_str[1], error_str[2]);  
    }
    return ret;
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
    max_thread = 0;

    // free allocations 
    for (i = 0; i < sim.max_object; i++) {
        free(sim.object[i]);
    }
    bzero(&sim, sizeof(sim));

    // done
    INFO("terminate complete\n");
}

void sim_gravity_set_obj_disp_r(object_t ** obj, int32_t max_object)
{
    double radius[max_object]; 
    double min_radius, max_radius, min_disp_r, max_disp_r, delta;
    int32_t i;

    #define MIN_DISP_R (8.)
    #define MAX_DISP_R (3. * MIN_DISP_R)
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

void sim_gravity_display(void)
{
    int32_t last_display, curr_display, next_display;

    last_display = DISPLAY_SIMULATION;
    next_display = DISPLAY_SIMULATION;
    curr_display = ((strlen(error_str[0]) > 0) ? DISPLAY_ERROR : DISPLAY_SIMULATION);

    while (true) {
        switch (curr_display) {
        case DISPLAY_SIMULATION:         
            next_display = sim_gravity_display_simulation(curr_display, last_display);
            break;
        case DISPLAY_SELECT:         
            next_display = sim_gravity_display_select(curr_display, last_display);
            break;
        case DISPLAY_HELP:         
            next_display = sim_gravity_display_help(curr_display, last_display);
            break;
        case DISPLAY_ERROR:         
            next_display = sim_gravity_display_error(curr_display, last_display);
            break;
        }

        if (sdl_quit || next_display == DISPLAY_TERMINATE) {
            break;
        }

        last_display = curr_display;
        curr_display = next_display;
    }
}

int32_t sim_gravity_display_simulation(int32_t curr_display, int32_t last_display)
{
    #define MAX_CIRCLE_RADIUS 51

    SDL_Rect       ctl_pane, sim_pane, below_sim_pane;
    int32_t        i;
    double         sim_pane_width;
    sdl_event_t *  event;
    char           str[100], str1[100];
    int32_t        next_display = -1;

    static SDL_Texture * circle_texture_tbl[MAX_CIRCLE_RADIUS][MAX_COLOR];

    //
    // loop until next_display has been set
    //

    while (next_display == -1) {
        //
        // short delay
        //

        usleep(5000);

        //
        // initialize the pane locations:
        // - sim_pane: the main simulation display area
        // - ctl_pane: the controls display area
        //

        if (sdl_win_width > sdl_win_height) {
            int32_t min_ctlpane_width = 18 * sdl_font[0].char_width;
            sim_pane_width = sdl_win_height;
            if (sim_pane_width + min_ctlpane_width > sdl_win_width) {
                sim_pane_width = sdl_win_width - min_ctlpane_width;
            }
            SDL_INIT_PANE(sim_pane, 
                          0, 0,  
                          sim_pane_width, sim_pane_width);
            SDL_INIT_PANE(below_sim_pane, 
                          0, sim_pane.h,
                          sim_pane_width, sdl_win_height-sim_pane.h);
            SDL_INIT_PANE(ctl_pane, 
                          sim_pane_width, 0, 
                          sdl_win_width-sim_pane_width, sdl_win_height);
        } else {
            sim_pane_width = sdl_win_width;
            SDL_INIT_PANE(sim_pane, 0, 0,  sim_pane_width, sim_pane_width);
            SDL_INIT_PANE(below_sim_pane, 0, sim_pane.h, sim_pane_width, sdl_win_height-sim_pane.h);
            SDL_INIT_PANE(ctl_pane, 0, 0, 0, 0);
        }
        sdl_event_init();

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
            int32_t    path_count, path_max, point_count, point_x, point_y;
            int64_t    path_idx;
            double     X, Y, X_ORIGIN, Y_ORIGIN;

            static SDL_Point  points[MAX_PATH+10];

            // display the object ...

            // determine display origin
            if (tracker_obj == -1) {
                X_ORIGIN = sim_x;
                Y_ORIGIN = sim_y;
            } else {
                X_ORIGIN = sim.object[tracker_obj]->X;
                Y_ORIGIN = sim.object[tracker_obj]->Y;
            }

            // determine object's display info
            // - position: disp_x, disp_y
            // - radius: disp_r
            // - color: disp_color
            disp_x = sim_pane.x + (obj->X - X_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
            disp_y = sim_pane.y + (obj->Y - Y_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
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

            // initialize:
            // - obj_tracker
            // - path_idx
            // - pat_max
            obj_tracker = (tracker_obj != -1 ? sim.object[tracker_obj] : NULL);
            path_idx = path_tail-1;
            path_max = path_disp_days;
            if (path_max < 0) {
                path_max = obj->MAX_PATH_DISP_DFLT;
            } 

            // be sure path_max is in valid range
            if (path_max < 0) {
                path_max = 0;
            }
            if (path_max > MAX_PATH_DISP_DAYS) {
                path_max = MAX_PATH_DISP_DAYS;
            }

            // if no path to display then continue
            if (path_max == 0 || path_idx < 0 || obj_tracker == obj) {
                continue;
            }

            // loop over path points, constructing the points[] array
            path_count = 0;
            point_count = 0;
            while (true) {
                if (path_idx < path_tail - MAX_PATH + 10) {
                    if (point_count > 0) {
                        point_count--;
                    }
                    break;
                }
                if (path_idx < 0 || path_count == path_max) {
                    break;
                }

                if (obj_tracker == NULL) {
                    X_ORIGIN = sim_x;
                    Y_ORIGIN = sim_y;
                } else {
                    X_ORIGIN = obj_tracker->PATH[path_idx%MAX_PATH].X;
                    Y_ORIGIN = obj_tracker->PATH[path_idx%MAX_PATH].Y;
                    if (X_ORIGIN == 0 && Y_ORIGIN == 0) {
                        break;
                    }
                }

                X = obj->PATH[path_idx%MAX_PATH].X;
                Y = obj->PATH[path_idx%MAX_PATH].Y;
                if (X == 0 && Y == 0) {
                    break;
                }

                point_x = sim_pane.x + (X - X_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
                point_y = sim_pane.y + (Y - Y_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);

                if (point_count == 0 || point_x != points[point_count-1].x || point_y != points[point_count-1].y) {
                    points[point_count].x = point_x;
                    points[point_count].y = point_y;
                    point_count++;
                }

                path_count++;
                path_idx--;
            }

            // display the lines that connect the points[] array
            SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255);
            SDL_RenderDrawLines(sdl_renderer, points, point_count);
        }

        // display sim_pane title
        sprintf(str, "WIDTH %0.2lf AU", sim_width/AU);
        sdl_render_text_font0(&sim_pane,  0, 0,  str, SDL_EVENT_NONE);
        if (sim_width >= 999*AU) {
            sprintf(str, "WIDTH %0.2lf LY", sim_width/LY);
            sdl_render_text_font0(&sim_pane,  1, 0,  str, SDL_EVENT_NONE);
        }

        // register sim_pane controls
        sdl_render_text_font0(&sim_pane,  0, -5,  " + ", SDL_EVENT_SIMPANE_ZOOM_IN);
        sdl_render_text_font0(&sim_pane,  2, -5,  " - ", SDL_EVENT_SIMPANE_ZOOM_OUT);
        sdl_event_register(SDL_EVENT_SIMPANE_MOUSE_MOTION, SDL_EVENT_TYPE_MOUSE_MOTION, &sim_pane);
        for (i = 0; i < sim.max_object; i++) {
            object_t * obj = sim.object[i];
            int32_t disp_x, disp_y;
            double X_ORIGIN, Y_ORIGIN;
            SDL_Rect mouse_click_rect;

            if (tracker_obj == -1) {
                X_ORIGIN = sim_x;
                Y_ORIGIN = sim_y;
            } else {
                X_ORIGIN = sim.object[tracker_obj]->X;
                Y_ORIGIN = sim.object[tracker_obj]->Y;
            }

            disp_x = sim_pane.x + (obj->X - X_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);
            disp_y = sim_pane.y + (obj->Y - Y_ORIGIN + sim_width/2) * (sim_pane_width / sim_width);

            if (disp_x < 0 || disp_x >= sim_pane_width || 
                disp_y < 0 || disp_y >= sim_pane_width) 
            {
                continue;
            }

            mouse_click_rect.x = disp_x - 40;
            mouse_click_rect.y = disp_y - 40;
            mouse_click_rect.w = 80;
            mouse_click_rect.h = 80;

            sdl_event_register(SDL_EVENT_SIMPANE_MOUSE_CLICK+i, SDL_EVENT_TYPE_MOUSE_CLICK, &mouse_click_rect);
        }

        // draw sim_pane border
        sdl_render_rect(&sim_pane, 3, sdl_pixel_rgba[GREEN]);

        // clear area below sim_pane, if it exists
        if (below_sim_pane.h > 0) {
            SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(sdl_renderer, &below_sim_pane);
        }

        //
        // CTLPANE ...
        //

        // clear ctl_pane
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(sdl_renderer, &ctl_pane);

        // display status lines
        sprintf(str, "%-4s %s", STATE_STR(state), dur2str(str1,sim.sim_time));
        sdl_render_text_font0(&ctl_pane, 10, 0, str, SDL_EVENT_NONE);

        if (DT >= 3600) {
            sprintf(str, "DT   %d HOURS", DT/3600);
        } else if (DT >= 60) {
            sprintf(str, "DT   %d MINUTES", DT/60);
        } else{
            sprintf(str, "DT   %d SECS", DT);
        }
        sdl_render_text_font0(&ctl_pane, 11, 0, str, SDL_EVENT_NONE);

        sprintf(str, "TRCK %s", tracker_obj != -1 ? sim.object[tracker_obj]->NAME : "OFF");
        sdl_render_text_font0(&ctl_pane, 12, 0, str, SDL_EVENT_NONE);

        if (path_disp_days == 0) {
            sdl_render_text_font0(&ctl_pane, 13, 0, "PATH OFF", SDL_EVENT_NONE);
        } else if (path_disp_days < 0) {
            if (sim.max_object == 0) {
                sdl_render_text_font0(&ctl_pane, 13, 0, "PATH UNKNOWN", SDL_EVENT_NONE);
            } else {
                int32_t days = sim.object[0]->MAX_PATH_DISP_DFLT;
                bool    varies = false;
                for (i = 1; i < sim.max_object; i++) {
                    if (sim.object[i]->MAX_PATH_DISP_DFLT != days) {
                        varies = true;
                        break;
                    }
                }
                if (varies) {
                    sdl_render_text_font0(&ctl_pane, 13, 0, "PATH VARIES", SDL_EVENT_NONE);
                } else {
                    if (days < 365.25) {
                        sprintf(str, "PATH %0.2lf Y", days/365.25);
                    } else {
                        sprintf(str, "PATH %0.0lf Y", days/365.25);
                    }
                    sdl_render_text_font0(&ctl_pane, 13, 0, str, SDL_EVENT_NONE);
                }
            }
        } else {
            if (path_disp_days < 365.25) {
                sprintf(str, "PATH %0.2lf Y", path_disp_days/365.25);
            } else {
                sprintf(str, "PATH %0.0lf Y", path_disp_days/365.25);
            }
            sdl_render_text_font0(&ctl_pane, 13, 0, str, SDL_EVENT_NONE);
        }

        if (state == STATE_RUN) {
            double years_per_sec;
            years_per_sec = (double)(sim.sim_time * 1000000 - perf_start_sim_time) / 
                            (microsec_timer() - perf_start_wall_time) /
                            (365.25 * 86400);
            sprintf(str, "RATE %0.3f Y/S", years_per_sec);
            sdl_render_text_font0(&ctl_pane, 14, 0, str, SDL_EVENT_NONE);
        }

        // display controls  
        sdl_render_text_ex(&ctl_pane, 0, 0, sim.sim_name, SDL_EVENT_NONE,
                           SDL_FIELD_COLS_UNLIMITTED, true, 0);
        sdl_render_text_font0(&ctl_pane,  2, 0,  "RUN",       SDL_EVENT_RUN);
        sdl_render_text_font0(&ctl_pane,  2, 7,  "STOP",      SDL_EVENT_STOP);
        sdl_render_text_font0(&ctl_pane,  4, 0,  "DT+",       SDL_EVENT_DT_PLUS);
        sdl_render_text_font0(&ctl_pane,  4, 7,  "DT-",       SDL_EVENT_DT_MINUS);
        sdl_render_text_font0(&ctl_pane,  6, 0,  "SELECT",    SDL_EVENT_SELECT);
        sdl_render_text_font0(&ctl_pane,  8, 0,  "PTHSTD",    SDL_EVENT_PATH_DISP_DEFAULT);
        sdl_render_text_font0(&ctl_pane,  8, 7,  "OFF",       SDL_EVENT_PATH_DISP_OFF);
        sdl_render_text_font0(&ctl_pane,  8, 11, " + ",       SDL_EVENT_PATH_DISP_PLUS);
        sdl_render_text_font0(&ctl_pane,  8, 15, " - ",       SDL_EVENT_PATH_DISP_MINUS);

        sdl_render_text_font0(&ctl_pane, -1, 0,  "HELP",      SDL_EVENT_HELP);
        sdl_render_text_font0(&ctl_pane, -1,-5,  "BACK",      SDL_EVENT_BACK);

        //
        // present the display
        //

        SDL_RenderPresent(sdl_renderer);

        //
        // handle events
        //

        event = sdl_poll_event();
        if (event != SDL_EVENT_NONE) {
            perf_start_sim_time = sim.sim_time * 1000000;
            perf_start_wall_time = microsec_timer() - 1;
        }
        switch (event->event) {
        case SDL_EVENT_RUN:
            state = STATE_RUN;
            break;
        case SDL_EVENT_STOP:
            state = STATE_STOP;
            break;
        case SDL_EVENT_BACK: 
        case SDL_EVENT_QUIT:
            next_display = DISPLAY_TERMINATE;  
            break;
        case SDL_EVENT_DT_PLUS:
            for (i = 0; i < MAX_VALID_DT-1; i++) {
                if (DT == valid_dt[i]) {
                    DT = valid_dt[i+1];
                    break;
                }
            }
            break;
        case SDL_EVENT_DT_MINUS:
            for (i = 1; i < MAX_VALID_DT; i++) {
                if (DT == valid_dt[i]) {
                    DT = valid_dt[i-1];
                    break;
                }
            }
            break;
        case SDL_EVENT_SELECT:
            state = STATE_STOP;
            next_display = DISPLAY_SELECT;
            break;
        case SDL_EVENT_PATH_DISP_DEFAULT:
            path_disp_days = -1;
            break;
        case SDL_EVENT_PATH_DISP_OFF:
            path_disp_days = 0;
            break;
        case SDL_EVENT_PATH_DISP_PLUS:
            if (path_disp_days <= 0) {
                path_disp_days = MIN_PATH_DISP_DAYS;
            } else  {
                path_disp_days *= 2;
            }
            if (path_disp_days < MIN_PATH_DISP_DAYS) {
                path_disp_days = MIN_PATH_DISP_DAYS;
            } else if (path_disp_days > MAX_PATH_DISP_DAYS) {
                path_disp_days = MAX_PATH_DISP_DAYS;
            }
            break;
        case SDL_EVENT_PATH_DISP_MINUS:
            if (path_disp_days <= 0) {
                path_disp_days = MAX_PATH_DISP_DAYS;
            } else  {
                path_disp_days /= 2;
            }
            if (path_disp_days < MIN_PATH_DISP_DAYS) {
                path_disp_days = MIN_PATH_DISP_DAYS;
            } else if (path_disp_days > MAX_PATH_DISP_DAYS) {
                path_disp_days = MAX_PATH_DISP_DAYS;
            }
            break;
        case SDL_EVENT_HELP:
            state = STATE_STOP;
            next_display = DISPLAY_HELP;
            break;
        case SDL_EVENT_SIMPANE_ZOOM_OUT:
            for (i = 0; i < MAX_VALID_SIM_WIDTH-1; i++) {
                if (IS_CLOSE(sim_width, valid_sim_width[i])) {
                    sim_width = valid_sim_width[i+1];
                    break;
                }
            }
            break;
        case SDL_EVENT_SIMPANE_ZOOM_IN:
            for (i = 1; i < MAX_VALID_SIM_WIDTH; i++) {
                if (IS_CLOSE(sim_width, valid_sim_width[i])) {
                    sim_width = valid_sim_width[i-1];
                    break;
                }
            }
            break;
        case SDL_EVENT_SIMPANE_MOUSE_CLICK ... SDL_EVENT_SIMPANE_MOUSE_CLICK_LAST: {
            int32_t obj_idx = event->event - SDL_EVENT_SIMPANE_MOUSE_CLICK;
            tracker_obj = (obj_idx == tracker_obj ? -1 : obj_idx);
            break; }
        case SDL_EVENT_SIMPANE_MOUSE_MOTION:
            if (tracker_obj == -1) {
                sim_x -= (event->mouse_motion.delta_x * (sim_width / sim_pane_width));
                sim_y -= (event->mouse_motion.delta_y * (sim_width / sim_pane_width));
            }
            break;
        }
    }

    //
    // return next_display
    //

    return next_display;
}

int32_t sim_gravity_display_select(int32_t curr_display, int32_t last_display)
{
    char  * location;
    char  * title;
    int32_t selection, i, ret;
    char    temp_str[PATH_MAX];
    char ** pathname = NULL;
    int32_t max_pathname = 0;

    // init
    location = "sim_gravity/";
    title    = "SIM GRAVITY - SELECTIONS";

    // obtain list of files from the appropriate location
    ret = list_files(location, &max_pathname, &pathname);

    // if failed then display error
    if (ret < 0) {
        sprintf(error_str[0], "List Files Failed");
        sprintf(error_str[1], "%s", location);
        error_str[2][0] = '\0';
        return DISPLAY_ERROR;
    }

    // bring up display to choose 
    char * basenames[max_pathname];
    for (i = 0; i < max_pathname; i++) {
        strcpy(temp_str, pathname[i]);
        basenames[i] = strdup(basename(temp_str));
    }
    sdl_display_choose_from_list(title, basenames, max_pathname, &selection);
    for (i = 0; i < max_pathname; i++) {
        free(basenames[i]);
    }

    // if a selction has been made then call sim_gravity_init to initialize 
    // the simulation from the selected pathname
    if (selection != -1) {
        int32_t ret = sim_gravity_init(pathname[selection]);
        if (ret != 0) {
            list_files_free(max_pathname, pathname);
            return DISPLAY_ERROR;
        }
    }

    // free the file list
    list_files_free(max_pathname, pathname);

    // return next_display
    return DISPLAY_SIMULATION;
}

int32_t sim_gravity_display_help(int32_t curr_display, int32_t last_display)
{
    // display the help text
    sdl_display_text(sim_gravity_help);

    // return next_display
    return DISPLAY_SIMULATION;
}

int32_t sim_gravity_display_error(int32_t curr_display, int32_t last_display)
{
    // display the error
    sdl_display_error(error_str[0], error_str[1], error_str[2]);

    // clear error strings
    error_str[0][0] = '\0';
    error_str[1][0] = '\0';
    error_str[2][0] = '\0';

    // go back to last display
    return DISPLAY_SIMULATION;
}

// -----------------  THREAD  ---------------------------------------------------

void * sim_gravity_thread(void * cx)
{
    int32_t thread_id = (long)cx;
    int32_t local_state;

    // all threads print starting message
    INFO("thread starting, thread_id=%d\n", thread_id);

    while (true) {
        // barrier
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

            int32_t i, k;
            double SUMX, SUMY, Mi, XDISTi, YDISTi, Ri, AX, AY, DX, DY, V1X, V1Y, V2X, V2Y, tmp;
            double EVALOBJ_TMP_X, EVALOBJ_TMP_Y;
            object_t * EVALOBJ, * OTHEROBJ;

            // each thread computes next position and next velocity of the objects
            // for which they are responsible
            for (k = thread_id; k < sim.max_object; k += max_thread) {
                EVALOBJ = sim.object[k];

                EVALOBJ_TMP_X = EVALOBJ->X + EVALOBJ->VX * DT / 2;
                EVALOBJ_TMP_Y = EVALOBJ->Y + EVALOBJ->VY * DT / 2;

                SUMX = SUMY = 0;
                for (i = 0; i < sim.max_object; i++) {
                    if (i == k) continue;

                    OTHEROBJ = sim.object[i];

                    Mi      = OTHEROBJ->MASS;
                    XDISTi  = OTHEROBJ->X - EVALOBJ_TMP_X;
                    YDISTi  = OTHEROBJ->Y - EVALOBJ_TMP_Y;
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

                EVALOBJ->NEXT_X = EVALOBJ->X + DX;
                EVALOBJ->NEXT_Y = EVALOBJ->Y + DY;
                EVALOBJ->NEXT_VX = V2X;
                EVALOBJ->NEXT_VY = V2Y;
            }

            // wait for all threads to finish updating the objects for which
            // they are responsible
            sim_gravity_barrier(thread_id);

            // thread zero does the final work
            if (thread_id == 0) {
                int32_t        day;
                bool           day_changed;
                static int32_t last_day;

                day = sim.sim_time/86400;
                day_changed = day != last_day;
                last_day = day;

                for (k = 0; k < sim.max_object; k++) {
                    object_t * obj = sim.object[k];

                    // store the object's new position and velocity
                    obj->X  = obj->NEXT_X;
                    obj->Y  = obj->NEXT_Y;
                    obj->VX = obj->NEXT_VX;
                    obj->VY = obj->NEXT_VY;

                    // if day has changed then save next entry in object's path
                    if (day_changed) {
                        obj->PATH[path_tail%MAX_PATH].X = obj->X;
                        obj->PATH[path_tail%MAX_PATH].Y = obj->Y;
                    }
                }

                // update simulation time
                sim.sim_time += DT;

                // if day has changed then update path_tail
                if (day_changed) {
                    __sync_fetch_and_add(&path_tail, 1);
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
