#define __STDC_FORMAT_MACROS

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
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

// xxx use this
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
bool           sdl_quit;

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


// sdl support: prototypes
void sdl_init(uint32_t w, uint32_t h);
void sdl_terminate(void);
void sdl_render_text_font0(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event);
void sdl_render_text_font1(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event);
void sdl_render_text_ex(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event, 
        int32_t field_cols, bool center, int32_t font_id);
void sdl_event_init(void);
int32_t sdl_poll_event(void);
void sdl_get_string(int32_t count, ...);
void sdl_render_rect(SDL_Rect * rect_arg, int32_t line_width, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);

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
void printmsg(char *fmt, ...);

// -----------------  TIME  ----------------------------------------------------------

#define MAX_TIME_STR    32

uint64_t microsec_timer(void);
time_t get_real_time_sec(void);
uint64_t get_real_time_us(void);
char * time2str(char * str, time_t time, bool gmt);

// -----------------  MISC  ----------------------------------------------------------

int32_t random_uniform(int32_t low, int32_t high);
int32_t random_triangular(int32_t low, int32_t high);
void wavelength_to_rgb(int32_t wl, uint8_t * r, uint8_t * g, uint8_t * b);

