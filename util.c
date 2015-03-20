#include "util.h"
#include "button_sound.h"

// -----------------  SDL SUPPORT  ---------------------------------------

// window init
//XXX #define SDL_WIN_WIDTH_INITIAL  1000
//XXX #define SDL_WIN_HEIGHT_INITIAL 900
#define SDL_WIN_WIDTH_INITIAL  400  
#define SDL_WIN_HEIGHT_INITIAL 640
#ifndef ANDROID 
#define SDL_FLAGS SDL_WINDOW_RESIZABLE //XXX ?
#else
#define SDL_FLAGS SDL_WINDOW_FULLSCREEN
#endif

// button sound
#define SDL_PLAY_BUTTON_SOUND() \
    do { \
        Mix_PlayChannel(-1, sdl_button_sound, 0); \
    } while (0)
Mix_Chunk * sdl_button_sound;

// events
#define EVENT_KEY_SHIFT   1
#define EVENT_KEY_ESC     2
#define EVENT_KEY_BS      3
#define EVENT_KEY_ENTER   4
SDL_Rect sdl_event_pos[SDL_MAX_EVENT];

// fonts
#define SDL_MAX_FONT                    2
#define SDL_PANE_COLS(p,fid)            ((p)->w / sdl_font[fid].char_width)
#define SDL_PANE_ROWS(p,fid)            ((p)->h / sdl_font[fid].char_height)
#ifndef ANDROID
#define SDL_FONT0_PATH                   "/usr/share/fonts/gnu-free/FreeMonoBold.ttf"
#define SDL_FONT0_PTSIZE                 20
#define SDL_FONT1_PATH                   "/usr/share/fonts/gnu-free/FreeMonoBold.ttf"
#define SDL_FONT1_PTSIZE                 32
#else
#define SDL_FONT0_PATH                   "/system/fonts/DroidSansMono.ttf"
#define SDL_FONT0_PTSIZE                 24
#define SDL_FONT1_PATH                   "/system/fonts/DroidSansMono.ttf"
#define SDL_FONT1_PTSIZE                 40 
#endif
struct {
    TTF_Font * font;
    int32_t    char_width;
    int32_t    char_height;
} sdl_font[SDL_MAX_FONT];

// getstring 
bool sdl_get_string_active;

// code
void sdl_init(void)
{
    // initialize Simple DirectMedia Layer  (SDL)
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
        FATAL("SDL_Init failed\n");
    }

    // create SDL Window and Renderer
    if (SDL_CreateWindowAndRenderer(SDL_WIN_WIDTH_INITIAL, SDL_WIN_HEIGHT_INITIAL, 
                                    SDL_FLAGS, &sdl_window, &sdl_renderer) != 0) 
    {
        FATAL("SDL_CreateWindowAndRenderer failed\n");
    }
    SDL_GetWindowSize(sdl_window, &sdl_win_width, &sdl_win_height);
    INFO("sdl_win_width=%d sdl_win_height=%d\n", sdl_win_width, sdl_win_height);

    // init button_sound
    if (Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        FATAL("Mix_OpenAudio failed\n");
    }
    sdl_button_sound = Mix_QuickLoad_WAV(button_sound_wav);
    if (sdl_button_sound == NULL) {
        FATAL("Mix_QuickLoadWAV failed\n");
    }
    Mix_VolumeChunk(sdl_button_sound,MIX_MAX_VOLUME/2);

    // initialize True Type Font
    if (TTF_Init() < 0) {
        FATAL("TTF_Init failed\n");
    }
    sdl_font[0].font = TTF_OpenFont(SDL_FONT0_PATH, SDL_FONT0_PTSIZE);
    if (sdl_font[0].font == NULL) {
        FATAL("failed TTF_OpenFont %s\n", SDL_FONT0_PATH);
    }
    TTF_SizeText(sdl_font[0].font, "X", &sdl_font[0].char_width, &sdl_font[0].char_height);
    sdl_font[1].font = TTF_OpenFont(SDL_FONT1_PATH, SDL_FONT1_PTSIZE);
    if (sdl_font[1].font == NULL) {
        FATAL("failed TTF_OpenFont %s\n", SDL_FONT1_PATH);
    }
    TTF_SizeText(sdl_font[1].font, "X", &sdl_font[1].char_width, &sdl_font[1].char_height);
}

void sdl_terminate(void)
{
    int32_t i;
    
    // cleanup
    Mix_FreeChunk(sdl_button_sound);
    Mix_CloseAudio();

    for (i = 0; i < SDL_MAX_FONT; i++) {
        TTF_CloseFont(sdl_font[i].font);
    }
    TTF_Quit();

    // xxx SDL_PumpEvents();  // helps on Android when terminating via return
    // xxx SDL_DestroyTexture(webcam[i].texture); 
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    // call exit,  on Android this causes the App (including the java shim) to terminate
    exit(0);
}

void sdl_render_text_font0(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event)
{
    sdl_render_text_ex(pane, row, col, str, event, SDL_PANE_COLS(pane,0)-col, false, 0);
}

void sdl_render_text_font1(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event)
{
    sdl_render_text_ex(pane, row, col, str, event, SDL_PANE_COLS(pane,1)-col, false, 1);
}

void sdl_render_text_ex(SDL_Rect * pane, int32_t row, int32_t col, char * str, int32_t event, 
        int32_t field_cols, bool center, int32_t font_id)
{
    SDL_Surface    * surface; 
    SDL_Texture    * texture; 
    SDL_Color        fg_color;
    SDL_Rect         pos;
    char             s[500];
    int32_t          slen;

    static SDL_Color fg_color_normal = {255,255,255}; 
    static SDL_Color fg_color_event  = {0,255,255}; 
    static SDL_Color bg_color        = {0,0,0}; 

    // if zero length string then nothing to do
    if (str[0] == '\0') {
        return;
    }

    // verify row, col, and field_cold
    if (row < 0 || row >= SDL_PANE_ROWS(pane,font_id) || 
        col < 0 || col >= SDL_PANE_COLS(pane,font_id) ||
        field_cols <= 0) 
    {
        return;
    }

    // reduce field_cols if necessary to stay in pane
    if (field_cols > SDL_PANE_COLS(pane,font_id) - col) {
         field_cols = SDL_PANE_COLS(pane,font_id) - col;
    }

    // make a copy of the str arg, and shorten if necessary
    strcpy(s, str);
    slen = strlen(s);
    if (slen > field_cols) {
        s[field_cols] = '\0';
        slen = field_cols;
    }

    // if centered then adjust col
    if (center) {
        col += (field_cols - slen) / 2;
    }

    // render the text to a surface0
    fg_color = (event != SDL_EVENT_NONE ? fg_color_event : fg_color_normal); 
    surface = TTF_RenderText_Shaded(sdl_font[font_id].font, s, fg_color, bg_color);
    if (surface == NULL) { 
        FATAL("TTF_RenderText_Shaded returned NULL\n");
    } 

    // determine the display location
    pos.x = pane->x + col * sdl_font[font_id].char_width;
    pos.y = pane->y + row * sdl_font[font_id].char_height;
    pos.w = surface->w;
    pos.h = surface->h;

    // create texture from the surface, and render the texture
    texture = SDL_CreateTextureFromSurface(sdl_renderer, surface); 
    SDL_RenderCopy(sdl_renderer, texture, NULL, &pos); 

    // clean up
    SDL_FreeSurface(surface); 
    SDL_DestroyTexture(texture); 

    // if there is a event then save the location for the event handler
    if (event != SDL_EVENT_NONE) {
        sdl_event_pos[event] = pos;
    }
}

void sdl_event_init(void)
{
    bzero(sdl_event_pos, sizeof(sdl_event_pos));
}

void sdl_poll_event(void)
{
    #define AT_POS(pos) ((ev.button.x >= (pos).x - 5) && \
                               (ev.button.x < (pos).x + (pos).w + 5) && \
                               (ev.button.y >= (pos).y - 5) && \
                               (ev.button.y < (pos).y + (pos).h + 5))

    #define SDL_WINDOWEVENT_STR(x) \
       ((x) == SDL_WINDOWEVENT_SHOWN        ? "SDL_WINDOWEVENT_SHOWN"        : \
        (x) == SDL_WINDOWEVENT_HIDDEN       ? "SDL_WINDOWEVENT_HIDDEN"       : \
        (x) == SDL_WINDOWEVENT_EXPOSED      ? "SDL_WINDOWEVENT_EXPOSED"      : \
        (x) == SDL_WINDOWEVENT_MOVED        ? "SDL_WINDOWEVENT_MOVED"        : \
        (x) == SDL_WINDOWEVENT_RESIZED      ? "SDL_WINDOWEVENT_RESIZED"      : \
        (x) == SDL_WINDOWEVENT_SIZE_CHANGED ? "SDL_WINDOWEVENT_SIZE_CHANGED" : \
        (x) == SDL_WINDOWEVENT_MINIMIZED    ? "SDL_WINDOWEVENT_MINIMIZED"    : \
        (x) == SDL_WINDOWEVENT_MAXIMIZED    ? "SDL_WINDOWEVENT_MAXIMIZED"    : \
        (x) == SDL_WINDOWEVENT_RESTORED     ? "SDL_WINDOWEVENT_RESTORED"     : \
        (x) == SDL_WINDOWEVENT_ENTER        ? "SDL_WINDOWEVENT_ENTER"        : \
        (x) == SDL_WINDOWEVENT_LEAVE        ? "SDL_WINDOWEVENT_LEAVE"        : \
        (x) == SDL_WINDOWEVENT_FOCUS_GAINED ? "SDL_WINDOWEVENT_FOCUS_GAINED" : \
        (x) == SDL_WINDOWEVENT_FOCUS_LOST   ? "SDL_WINDOWEVENT_FOCUS_LOST"   : \
        (x) == SDL_WINDOWEVENT_CLOSE        ? "SDL_WINDOWEVENT_CLOSE"        : \

    SDL_Event ev;

    while (true) {
        // get the next event, break out of loop if no event
        if (SDL_PollEvent(&ev) == 0) {
            break;
        }

        // process the SDL event, this code
        // - sets sdl_event and sdl_quit_event
        // - updates sdl_win_width, sdl_win_height, sdl_win_minimized
        switch (ev.type) {
        case SDL_MOUSEBUTTONDOWN: {
            int32_t i;
            DEBUG("MOUSE DOWN which=%d button=%s state=%s x=%d y=%d\n",
                   ev.button.which,
                   (ev.button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                    ev.button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                    ev.button.button == SDL_BUTTON_RIGHT  ? "RIGHT" :
                                                            "???"),
                   (ev.button.state == SDL_PRESSED  ? "PRESSED" :
                    ev.button.state == SDL_RELEASED ? "RELEASED" :
                                                      "???"),
                   ev.button.x,
                   ev.button.y);

            if (ev.button.button != SDL_BUTTON_LEFT) {
                break;
            }

            for (i = 0; i < SDL_MAX_EVENT; i++) {
                if (AT_POS(sdl_event_pos[i])) {
                    DEBUG("got event EVENT %d\n", i);
                    sdl_event = i;
                    SDL_PLAY_BUTTON_SOUND();
                    break;
                }
            }
            break; }

        case SDL_KEYDOWN: {
            int32_t  key = ev.key.keysym.sym;
            bool shift = (ev.key.keysym.mod & KMOD_SHIFT) != 0;

            if (!sdl_get_string_active) {
                break;
            }

            if (key == 27) {
                sdl_event = EVENT_KEY_ESC;
            } else if (key == 8) {
                sdl_event = EVENT_KEY_BS;
            } else if (key == 13) {
                sdl_event = EVENT_KEY_ENTER;
            } else if (!shift && ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9'))) {
                sdl_event = key;
            } else if (shift && (key >= 'a' && key <= 'z')) {
                sdl_event = 'A' + (key - 'a');
            } else if (shift && key == '-') {
                sdl_event = '_';
            } else {
                break;
            }
            SDL_PLAY_BUTTON_SOUND();
            break; }

       case SDL_WINDOWEVENT: {
            DEBUG("got event SDL_WINOWEVENT - %s\n", SDL_WINDOWEVENT_STR(ev.window.event));
            switch (ev.window.event)  {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                sdl_win_width = ev.window.data1;
                sdl_win_height = ev.window.data2;
                SDL_PLAY_BUTTON_SOUND();
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                sdl_win_minimized = true;
                SDL_PLAY_BUTTON_SOUND();
                break;
            case SDL_WINDOWEVENT_RESTORED:
                sdl_win_minimized = false;
                SDL_PLAY_BUTTON_SOUND();
                break;
            }
            break; }

        case SDL_QUIT: {
            DEBUG("got event SDL_QUIT\n");
            sdl_quit_event = true;
            SDL_PLAY_BUTTON_SOUND();
            break; }

        case SDL_MOUSEMOTION: {
            break; }

        default: {
            DEBUG("got event %d - not supported\n", ev.type);
            break; }
        }

        // break if sdl_event or sdl_quit_event is set
        if (sdl_event != SDL_EVENT_NONE || sdl_quit_event) {
            break; 
        }
    }
}

bool sdl_get_string(char * prompt_str, char * ret_str, size_t ret_str_size)
{
    bool     ret            = false;
    char     input_str[100] = "";
    int32_t  input_str_len  = 0;
    bool     shift          = false;
    int32_t  sdl_event_save = sdl_event;

    SDL_Rect keybdpane; 
    char     display_str[500];

    // init
    sdl_event = SDL_EVENT_NONE;
    sdl_get_string_active = true;

    // loop until string entered, or escape
    while (true) {
        // init
        SDL_INIT_PANE(keybdpane, 0, 0, sdl_win_width, sdl_win_height);
        sdl_event_init();

        // clear window
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // draw keyboard
        static char * row_chars_unshift[4] = { "1234567890_",
                                               "qwertyuiop",
                                               "asdfghjkl",
                                               "zxcvbnm" };
        static char * row_chars_shift[4]   = { "1234567890_",
                                               "QWERTYUIOP",
                                               "ASDFGHJKL",
                                               "ZXCVBNM" };
        char ** row_chars;
        int32_t r, c, i, j;
        char str[2];

        row_chars = (!shift ? row_chars_unshift : row_chars_shift);
        r = SDL_PANE_ROWS(&keybdpane,1) / 2 - 4;
        c = (SDL_PANE_COLS(&keybdpane,1) - 33) / 2;

        for (i = 0; i < 4; i++) {
            for (j = 0; row_chars[i][j] != '\0'; j++) {
                str[0] = row_chars[i][j];
                str[1] = '\0';
                sdl_render_text_ex(&keybdpane, r+2*i, c+3*j, str, str[0], 1, false, 1);
            }
        }

        sdl_render_text_ex(&keybdpane, r+8, c+0,  "SHIFT", EVENT_KEY_SHIFT, 5, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+8,  "ESC",   EVENT_KEY_ESC,   3, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+14, "BS",    EVENT_KEY_BS,    2, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+19, "ENTER", EVENT_KEY_ENTER, 5, false, 1);

        // xxx horizontal scroll on long inputs
        sprintf(display_str, "%s: %s%c", 
                prompt_str, 
                input_str, 
                ((microsec_timer() % 1000000) > 500000) ? '_' : ' ');
        sdl_render_text_ex(&keybdpane, r+10, c+0, display_str, SDL_EVENT_NONE, SDL_FIELD_COLS_UNLIMITTED, false, 1);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // poll for events
        sdl_poll_event();

        // handle events 
        if (sdl_event >= 0x20 && sdl_event <= 0x7e) {
            if (input_str_len < sizeof(input_str) - 1) {
                input_str[input_str_len++] = sdl_event;
                input_str[input_str_len]   = '\0';
            }
        } else if (sdl_event == EVENT_KEY_SHIFT) {
            shift = !shift;
        } else if (sdl_event == EVENT_KEY_ESC || sdl_quit_event) {
            ret = false;
            break;
        } else if (sdl_event == EVENT_KEY_BS) {
            if (input_str_len > 0) {
                input_str_len--;
                input_str[input_str_len] = '\0';
            }
        } else if (sdl_event == EVENT_KEY_ENTER) {
            strncpy(ret_str, input_str, ret_str_size);
            ret_str[ret_str_size-1] = '\0';
            ret = true;
            break;
        }
        sdl_event = SDL_EVENT_NONE;

        // short sleep
        usleep(1000);
    }

    // done
    sdl_event_init();
    sdl_event = sdl_event_save;
    sdl_get_string_active = false;
    return ret;
}

// -----------------  CONFIG READ / WRITE  -------------------------------

int config_read(char * config_path, config_t * config, int config_version)
{
    FILE * fp;
    int    i, version=0;
    char * name;
    char * value;
    char * saveptr;
    char   s[100] = "";

    // open config_file and verify version, 
    // if this fails then write the config file with default values
    if ((fp = fopen(config_path, "re")) == NULL ||
        fgets(s, sizeof(s), fp) == NULL ||
        sscanf(s, "VERSION %d", &version) != 1 ||
        version != config_version)
    {
        if (fp != NULL) {
            fclose(fp);
        }
        INFO("creating default config file %s, version=%d\n", config_path, config_version);
        return config_write(config_path, config, config_version);
    }

    // read config entries
    while (fgets(s, sizeof(s), fp) != NULL) {
        name = strtok_r(s, " \n", &saveptr);
        if (name == NULL || name[0] == '#') {
            continue;
        }

        value = strtok_r(NULL, " \n", &saveptr);
        if (value == NULL) {
            value = "";
        }

        for (i = 0; config[i].name[0]; i++) {
            if (strcmp(name, config[i].name) == 0) {
                strcpy(config[i].value, value);
                break;
            }
        }
    }

    // close
    fclose(fp);
    return 0;
}

int config_write(char * config_path, config_t * config, int config_version)
{
    FILE * fp;
    int    i;

    // open
    fp = fopen(config_path, "we");  // mode: truncate-or-create, close-on-exec
    if (fp == NULL) {
        ERROR("failed to write config file %s, %s\n", config_path, strerror(errno));
        return -1;
    }

    // write version
    fprintf(fp, "VERSION %d\n", config_version);

    // write name/value pairs
    for (i = 0; config[i].name[0]; i++) {
        fprintf(fp, "%-20s %s\n", config[i].name, config[i].value);
    }

    // close
    fclose(fp);
    return 0;
}

// -----------------  LOGGING & PRINTMSG  ---------------------------------

#ifndef ANDROID

FILE * logmsg_fp             = NULL;
FILE * logmsg_fp_old         = NULL;
size_t logmsg_file_size      = 0;
char   logmsg_file_name[100] = "stderr";
bool   logmsg_disabled       = false;
bool   logmsg_init_called    = false;

void logmsg_init(char * file_name)
{
    struct stat buf;

    // don't support calling this routine more than once
    if (logmsg_init_called) {
        FATAL("logmsg_init called multiple times\n");
    }
    logmsg_init_called = true;

    // save copy of file_name
    strcpy(logmsg_file_name, file_name);

    // determine logmsg_disabled flag, if set then return
    logmsg_disabled = (strcmp(logmsg_file_name, "none") == 0);
    if (logmsg_disabled) {
        return;
    }

    // if logmsg_file_name is stderr then set logmsg_fp to NULL and return
    if (strcmp(logmsg_file_name, "stderr") == 0) {
        logmsg_fp = NULL;
        return;
    }

    // logging is to a file:
    // - open the file
    // - determine its size
    // - set line buffering
    logmsg_fp = fopen(logmsg_file_name, "ae");   // mode: append, close-on-exec
    if (logmsg_fp == NULL) {
        FATAL("failed to create logmsg file %s, %s\n", logmsg_file_name, strerror(errno));
    }
    if (stat(logmsg_file_name, &buf) != 0) {
        FATAL("failed to stat logmsg file %s, %s\n", logmsg_file_name, strerror(errno));
    }
    logmsg_file_size = buf.st_size;
    setlinebuf(logmsg_fp);
}

void logmsg(char *lvl, const char *func, char *fmt, ...) 
{
    va_list ap;
    char    msg[1000];
    int     len, cnt;
    char    time_str[MAX_TIME_STR];

    // if disabled then 
    // - print FATAL msg to stderr
    // - return
    // endif
    if (logmsg_disabled) {
        if (strcmp(lvl, "FATAL") == 0) {
            va_start(ap, fmt);
            vfprintf(stderr, fmt, ap);
            va_end(ap);
        }
        return;
    }

    // construct msg
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // remove terminating newline
    len = strlen(msg);
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

    // check if logging to file vs stderr
    if (logmsg_fp != NULL) {
        // logging to file

        // print the preamble and msg
        cnt = fprintf(logmsg_fp, "%s %s %s: %s\n",
                      time2str(time_str, get_real_time_sec(), false),
                      lvl, func, msg);

        // keep track of file size
        logmsg_file_size += cnt;

        // if file size greater than max then rename file to file.old, and create new file
        if (logmsg_file_size > MAX_LOGMSG_FILE_SIZE) {
            char   dot_old[100];
            FILE * new_fp;

            if (logmsg_fp_old) {
                fclose(logmsg_fp_old);
            }
            logmsg_fp_old = logmsg_fp;

            sprintf(dot_old, "%s.old", logmsg_file_name);
            rename(logmsg_file_name, dot_old);

            new_fp = fopen(logmsg_file_name, "we");
            if (new_fp == NULL) {
                FATAL("failed to create logmsg file %s, %s\n", logmsg_file_name, strerror(errno));
            }
            setlinebuf(new_fp);

            logmsg_fp = new_fp;
            logmsg_file_size = 0;
        }
    } else {
        // logging to stderr
        cnt = fprintf(stderr, "%s %s %s: %s\n",
                      time2str(time_str, get_real_time_sec(), false),
                      lvl, func, msg);
    }
}

void printmsg(char *fmt, ...) 
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

#else

void logmsg_init(char * file_name)
{
    // nothing to do here for Android,
    // logging is always performed
}

void logmsg(char *lvl, const char *func, char *fmt, ...) 
{
    va_list ap;
    char    msg[1000];
    int     len;
    char    time_str[MAX_TIME_STR];

    // construct msg
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    // remove terminating newline
    len = strlen(msg);
    if (len > 0 && msg[len-1] == '\n') {
        msg[len-1] = '\0';
        len--;
    }

    // log the message
    if (strcmp(lvl, "INFO") == 0) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "%s %s %s: %s\n",
                    time2str(time_str, time(NULL), false),
                    lvl, func, msg);
    } else if (strcmp(lvl, "WARN") == 0) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "%s %s %s: %s\n",
                    time2str(time_str, time(NULL), false),
                    lvl, func, msg);
    } else if (strcmp(lvl, "FATAL") == 0) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                        "%s %s %s: %s\n",
                        time2str(time_str, time(NULL), false),
                        lvl, func, msg);
    } else if (strcmp(lvl, "DEBUG") == 0) {
        SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                     "%s %s %s: %s\n",
                     time2str(time_str, time(NULL), false),
                     lvl, func, msg);
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s %s %s: %s\n",
                     time2str(time_str, time(NULL), false),
                     lvl, func, msg);
    }
}

void printmsg(char *fmt, ...) 
{
    va_list ap;

    va_start(ap, fmt);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, ap);
    va_end(ap);
}

#endif

// -----------------  TIME UTILS  -----------------------------------------

uint64_t microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
}

time_t get_real_time_sec(void)
{
    return get_real_time_us() / 1000000;
}

uint64_t get_real_time_us(void)
{
    struct timespec ts;
    uint64_t us;
    static uint64_t last_us;

    clock_gettime(CLOCK_REALTIME,&ts);
    us = ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);

    if (us <= last_us) {
        us = last_us + 1;
    }
    last_us = us;

    return us;
}

char * time2str(char * str, time_t time, bool gmt) 
{
    struct tm tm;

    if (gmt) {
        gmtime_r(&time, &tm);
        snprintf(str, MAX_TIME_STR,
                "%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d GMT",
                tm.tm_mon+1, tm.tm_mday, tm.tm_year%100,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else {
        localtime_r(&time, &tm);
        snprintf(str, MAX_TIME_STR,
                "%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
                tm.tm_mon+1, tm.tm_mday, tm.tm_year%100,
                tm.tm_hour, tm.tm_min, tm.tm_sec);
    }


    return str;
}

