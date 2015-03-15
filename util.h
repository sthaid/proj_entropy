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
#include <assert.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

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
#define SDL_EVENT_NONE 0
#define SDL_MAX_EVENT  256 
int32_t  sdl_event;
bool     sdl_quit_event;

// sdl support: prototypes
void sdl_init(void);
void sdl_terminate(void);
void sdl_render_text_font0(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event);
void sdl_render_text_font1(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event);
void sdl_render_text_ex(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event, 
        int32_t field_cols, bool center, int32_t font_id);
void sdl_event_init(void);
void sdl_poll_event(void);
bool sdl_get_string(char * prompt_str, char * ret_str, size_t ret_str_size);

// -----------------  CONFIG READ/WRITE  --------------------------------------------

#define MAX_CONFIG_VALUE_STR 100

typedef struct {
    const char * name;
    char         value[MAX_CONFIG_VALUE_STR];
} config_t;

int config_read(char * config_path, config_t * config, int config_version);
int config_write(char * config_path, config_t * config, int config_version);

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

// #define TIMESPEC_TO_US(ts) ((uint64_t)(ts)->tv_sec * 1000000 + (ts)->tv_nsec / 1000)
// #define TIMEVAL_TO_US(tv)  ((uint64_t)(tv)->tv_sec * 1000000 + (tv)->tv_usec)

#define MAX_TIME_STR    32

uint64_t microsec_timer(void);
time_t get_real_time_sec(void);
uint64_t get_real_time_us(void);
char * time2str(char * str, time_t time, bool gmt);

// -----------------  TIMING CODE PATH DURATION  -------------------------------------

#ifdef DEBUG_TIMING

typedef struct {
    const char * name;
    uint64_t     min;
    uint64_t     max;
    uint64_t     sum;
    uint64_t     count;
    uint64_t     begin_us;
    uint64_t     last_print_us;
    uint64_t     print_intvl_us;
} timing_t;

#define TIMING_DECLARE(tmg, pr_intvl) \
    static timing_t tmg = { #tmg, UINT64_MAX, 0, 0, 0, 0, 0, pr_intvl }

#define TIMING_BEGIN(tmg) \
    do {\
        (tmg)->begin_us = microsec_timer(); \
    } while (0)

#define TIMING_END(tmg) \
    do { \
        uint64_t curr_us = microsec_timer(); \
        uint64_t dur_us = curr_us - (tmg)->begin_us; \
        if (dur_us < (tmg)->min) { \
            (tmg)->min = dur_us; \
        } \
        if (dur_us > (tmg)->max) { \
            (tmg)->max = dur_us; \
        } \
        (tmg)->sum += dur_us; \
        (tmg)->count++; \
        if ((tmg)->last_print_us == 0) { \
            (tmg)->last_print_us = curr_us; \
        } else if (curr_us - (tmg)->last_print_us > (tmg)->print_intvl_us) { \
            NOTICE("TIMING %s avg=%d.%3.3d min=%d.%3.3d max=%d.%3.3d count=%d\n", \
                   (tmg)->name, \
                   (int)(((tmg)->sum/(tmg)->count) / 1000000), \
                   (int)(((tmg)->sum/(tmg)->count) % 1000000) / 1000, \
                   (int)((tmg)->min / 1000000), \
                   (int)((tmg)->min % 1000000) / 1000, \
                   (int)((tmg)->max / 1000000), \
                   (int)((tmg)->max % 1000000) / 1000, \
                   (int)((tmg)->count)); \
            (tmg)->min           = UINT64_MAX; \
            (tmg)->max           = 0; \
            (tmg)->sum           = 0; \
            (tmg)->count         = 0; \
            (tmg)->begin_us      = 0; \
            (tmg)->last_print_us = curr_us; \
        } \
    } while (0)

#else

#define TIMING_DECLARE(tmg, pr_intvl)

#define TIMING_BEGIN(tmg) \
    do { \
    } while (0)

#define TIMING_END(tmg) \
    do { \
    } while (0)

#endif

