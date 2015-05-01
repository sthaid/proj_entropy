#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h> 
#include <math.h> 
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

// -----------------  SIMULATIONS  --------------------------------------------------

void sim_container(void);
void sim_gravity(void);

// -----------------  VERSION  ------------------------------------------------------

// xxx use this, and build date
#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#define VERSION ( { version_t v = { VERSION_MAJOR, VERSION_MINOR }; v; } );

typedef struct {
    int major;
    int minor;
} version_t;

// -----------------  SDL SUPPORT  --------------------------------------------------

// window state
SDL_Window   * sdl_window;
SDL_Renderer * sdl_renderer;
int32_t        sdl_win_width;
int32_t        sdl_win_height;
bool           sdl_win_minimized;

// program quit requested
bool sdl_quit;

// fonts
#define SDL_PANE_COLS(p,fid)  ((p)->w / sdl_font[fid].char_width)
#define SDL_PANE_ROWS(p,fid)  ((p)->h / sdl_font[fid].char_height)
#define SDL_MAX_FONT 2
typedef struct {
    TTF_Font * font;
    int32_t    char_width;
    int32_t    char_height;
} sdl_font_t;
sdl_font_t sdl_font[SDL_MAX_FONT];

// render_text
#define SDL_FIELD_COLS_UNLIMITTED 999
#define SDL_INIT_PANE(r,_x,_y,_w,_h) \
    do { \
        (r).x = (_x); \
        (r).y = (_y); \
        (r).w = (_w); \
        (r).h = (_h); \
    } while (0)

// events
#define SDL_EVENT_NONE              0
#define SDL_EVENT_QUIT              1
#define SDL_EVENT_WIN_SIZE_CHANGE   2
#define SDL_EVENT_WIN_MINIMIZED     3
#define SDL_EVENT_WIN_RESTORED      4
#define SDL_EVENT_USER_START        10  
#define SDL_EVENT_USER_END          255
#define SDL_EVENT_MAX               256 

// event types
#define SDL_EVENT_TYPE_NONE         0
#define SDL_EVENT_TYPE_TEXT         1
#define SDL_EVENT_TYPE_MOUSE_CLICK  2
#define SDL_EVENT_TYPE_MOUSE_MOTION 3

// event data structure
typedef struct {
    int32_t event;
    union {
        struct {
            int32_t x;
            int32_t y;
        } mouse_click;
        struct {
            int32_t delta_x;
            int32_t delta_y;;
        } mouse_motion;
    };
} sdl_event_t;

// colors
#define PURPLE     0 
#define BLUE       1
#define LIGHT_BLUE 2
#define GREEN      3
#define YELLOW     4
#define ORANGE     5
#define RED        6
#define GRAY       7
#define PINK       8
#define WHITE      9
#define MAX_COLOR  10

#define COLOR_STR_TO_COLOR(cs) \
    (strcmp((cs), "PURPLE")     == 0 ? PURPLE     : \
     strcmp((cs), "BLUE")       == 0 ? BLUE       : \
     strcmp((cs), "LIGHT_BLUE") == 0 ? LIGHT_BLUE : \
     strcmp((cs), "GREEN")      == 0 ? GREEN      : \
     strcmp((cs), "YELLOW")     == 0 ? YELLOW     : \
     strcmp((cs), "ORANGE")     == 0 ? ORANGE     : \
     strcmp((cs), "RED")        == 0 ? RED        : \
     strcmp((cs), "GRAY")       == 0 ? GRAY       : \
     strcmp((cs), "PINK")       == 0 ? PINK       : \
     strcmp((cs), "WHITE")      == 0 ? WHITE        \
                                     : WHITE)

extern uint32_t sdl_pixel_rgba[];

// sdl support: prototypes
void sdl_init(uint32_t w, uint32_t h);
void sdl_terminate(void);
void sdl_event_init(void);
void sdl_event_register(int32_t event_id, int32_t event_type, SDL_Rect * pos);
sdl_event_t * sdl_poll_event(void);
void sdl_play_event_sound(void);
void sdl_render_text_font0(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event);
void sdl_render_text_font1(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event);
void sdl_render_text_ex(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event, 
        int32_t field_cols, bool center, int32_t font_id);
void sdl_render_rect(SDL_Rect * rect, int32_t line_width, uint32_t rgba);
SDL_Texture * sdl_create_filled_circle_texture(int32_t radius, uint32_t rgba);
void sdl_render_circle(int32_t x, int32_t y, SDL_Texture * circle_texture);
void sdl_display_get_string(int32_t count, ...);
void sdl_display_text(char * title, char **lines);
void sdl_display_choose_from_list(char * title_str, char ** choices, int32_t max_choices, int32_t * selection);

// -----------------  PTHREAD ADDITIONS FOR ANDROID  --------------------------------

#ifdef ANDROID

#define pthread_barrier_init    Pthread_barrier_init
#define pthread_barrier_destroy Pthread_barrier_destroy
#define pthread_barrier_wait    Pthread_barrier_wait
#define pthread_barrier_t       Pthread_barrier_t
#define pthread_barrierattr_t   Pthread_barrierattr_t

typedef void * Pthread_barrierattr_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint64_t       count;
    uint64_t       current;;
} Pthread_barrier_t;

int Pthread_barrier_wait(Pthread_barrier_t *barrier);
int Pthread_barrier_destroy(Pthread_barrier_t *barrier);
int Pthread_barrier_init(Pthread_barrier_t *barrier, const Pthread_barrierattr_t *attr, unsigned count);

#endif

// -----------------  CONFIG READ/WRITE  --------------------------------------------

#define MAX_CONFIG_VALUE_STR 100

typedef struct {
    uint32_t version;
    struct {
        const char * name;
        char         value[MAX_CONFIG_VALUE_STR];
    } ent[];
} config_t;

int config_read(char * filename, config_t * config);
int config_write(char * filename, config_t * config);

// -----------------  LOGGING  -------------------------------------------------------

#define INFO(fmt, args...) \
    do { \
        logmsg("INFO", __func__, fmt, ## args); \
    } while (0)
#define WARN(fmt, args...) \
    do { \
        logmsg("WARN", __func__, fmt, ## args); \
    } while (0)
#define ERROR(fmt, args...) \
    do { \
        logmsg("ERROR", __func__, fmt, ## args); \
    } while (0)
#define FATAL(fmt, args...) \
    do { \
        logmsg("FATAL", __func__, fmt, ## args); \
        exit(1); \
    } while (0)
#ifdef DEBUG_PRINTS
  #define DEBUG(fmt, args...) \
      do { \
          logmsg("DEBUG", __func__, fmt, ## args); \
      } while (0)
#else
  #define DEBUG(fmt, args...) 
#endif

#define PRINTF(fmt, args...) \
    do { \
        printmsg(fmt, ## args); \
    } while (0)

#define MAX_LOGMSG_FILE_SIZE 0x100000

void logmsg_init(char * logmsg_file);
void logmsg(char * lvl, const char * func, char * fmt, ...) __attribute__ ((format (printf, 3, 4)));
void printmsg(char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

// -----------------  TIME  ----------------------------------------------------------

#define MAX_TIME_STR    32

uint64_t microsec_timer(void);
time_t get_real_time_sec(void);
uint64_t get_real_time_us(void);
char * time2str(char * str, time_t time, bool gmt);
char * dur2str(char * str, int64_t duration);

// -----------------  FILE ACCESS  ---------------------------------------------------

typedef void file_t;

void list_files(char * location, int32_t * max, char *** pathnames);
void list_files_free(int32_t max, char ** pathnames);
file_t * open_file(char * pathname);
char * read_file_line(file_t * file);
void close_file(file_t * file);

// -----------------  MISC  ----------------------------------------------------------

int32_t random_uniform(int32_t low, int32_t high);
int32_t random_triangular(int32_t low, int32_t high);
void wavelength_to_rgb(int32_t wl, uint8_t * r, uint8_t * g, uint8_t * b);

