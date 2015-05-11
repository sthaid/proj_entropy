#include "util.h"
#include "button_sound.h"

#ifndef ANDROID
#include <dirent.h>
#else
#include <android/asset_manager_jni.h>
#endif

// -----------------  SDL SUPPORT  ---------------------------------------

// - - - - - - - - -  SDL INIT & TERMINATE  - - - - - - - - - - - - - - - 

// window init
#ifndef ANDROID 
#define SDL_FLAGS SDL_WINDOW_RESIZABLE
#else
#define SDL_FLAGS SDL_WINDOW_FULLSCREEN
#endif


static Mix_Chunk * sdl_button_sound;

void sdl_init(uint32_t w, uint32_t h)
{
    char  * font0_path, * font1_path;
    int32_t font0_ptsize, font1_ptsize;

    // initialize Simple DirectMedia Layer  (SDL)
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
        FATAL("SDL_Init failed\n");
    }

    // create SDL Window and Renderer
    if (SDL_CreateWindowAndRenderer(w, h, SDL_FLAGS, &sdl_window, &sdl_renderer) != 0) {
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

    font0_path = "fonts/FreeMonoBold.ttf";
    font0_ptsize = 40;
    font1_path = "fonts/FreeMonoBold.ttf";
    font1_ptsize = 60;

    sdl_font[0].font = TTF_OpenFont(font0_path, font0_ptsize);
    if (sdl_font[0].font == NULL) {
        FATAL("failed TTF_OpenFont %s\n", font0_path);
    }
    TTF_SizeText(sdl_font[0].font, "X", &sdl_font[0].char_width, &sdl_font[0].char_height);

    sdl_font[1].font = TTF_OpenFont(font1_path, font1_ptsize);
    if (sdl_font[1].font == NULL) {
        FATAL("failed TTF_OpenFont %s\n", font1_path);
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

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    // call exit,  on Android this causes the App (including the java shim) to terminate
    exit(0);
}

// - - - - - - - - -  EVENT HANDLING  - - - - - - - - - - - - - - - - - - 

typedef struct {
    SDL_Rect pos;
    int32_t type;
} sdl_event_reg_t;

static sdl_event_reg_t sdl_event_reg_tbl[SDL_EVENT_MAX];
static bool            sdl_enable_keybd_events;

void sdl_event_init(void)
{
    bzero(sdl_event_reg_tbl, sizeof(sdl_event_reg_tbl));
}

void sdl_event_register(int32_t event_id, int32_t event_type, SDL_Rect * pos)
{
    sdl_event_reg_tbl[event_id].pos  = *pos;
    sdl_event_reg_tbl[event_id].type = event_type;
}

sdl_event_t * sdl_poll_event(void)
{
    #define AT_POS(X,Y,pos,pad) (((X) >= (pos).x - (pad)) && \
                                 ((X) < (pos).x + (pos).w + (pad)) && \
                                 ((Y) >= (pos).y - (pad)) && \
                                 ((Y) < (pos).y + (pos).h + (pad)))

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

    #define SDL_EVENT_KEY_SHIFT         128
    #define SDL_EVENT_KEY_BS            129
    #define SDL_EVENT_KEY_TAB           130
    #define SDL_EVENT_KEY_ENTER         131
    #define SDL_EVENT_KEY_ESC           132
    #define SDL_EVENT_FIELD_SELECT      140

    #define MOUSE_BUTTON_STATE_NONE     0
    #define MOUSE_BUTTON_STATE_DOWN     1
    #define MOUSE_BUTTON_STATE_MOTION   2

    #define MOUSE_BUTTON_STATE_RESET \
        do { \
            mouse_button_state = MOUSE_BUTTON_STATE_NONE; \
            mouse_button_motion_event = SDL_EVENT_NONE; \
            mouse_button_x = 0; \
            mouse_button_y = 0; \
        } while (0)

    SDL_Event ev;
    int32_t i;

    static sdl_event_t event;
    static int32_t     mouse_button_state;  // xxx how to ensure this is reset
    static int32_t     mouse_button_motion_event;
    static int32_t     mouse_button_x;
    static int32_t     mouse_button_y;

    bzero(&event, sizeof(event));
    event.event = SDL_EVENT_NONE;

    while (true) {
        // get the next event, break out of loop if no event
        if (SDL_PollEvent(&ev) == 0) {
            break;
        }

        // process the SDL event, this code
        // - sets event and sdl_quit
        // - updates sdl_win_width, sdl_win_height, sdl_win_minimized
        switch (ev.type) {
        case SDL_MOUSEBUTTONDOWN: {
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

            // if not the left button then get out
            if (ev.button.button != SDL_BUTTON_LEFT) {
                break;
            }

            // reset mouse_button_state
            MOUSE_BUTTON_STATE_RESET;

            // check for text event
            for (i = 0; i < SDL_EVENT_MAX; i++) {
                if (sdl_event_reg_tbl[i].type == SDL_EVENT_TYPE_TEXT &&
                    AT_POS(ev.button.x, ev.button.y, sdl_event_reg_tbl[i].pos, 5)) 
                {
                    event.event = i;
                    break;
                }
            }
            if (event.event != SDL_EVENT_NONE) {
                break;
            }

            // it is not a text event, so set MOUSE_BUTTON_STATE_DOWN
            mouse_button_state = MOUSE_BUTTON_STATE_DOWN;
            mouse_button_x = ev.button.x;
            mouse_button_y = ev.button.y;
            break; }

        case SDL_MOUSEBUTTONUP: {
            DEBUG("MOUSE UP which=%d button=%s state=%s x=%d y=%d\n",
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

            // if not the left button then get out
            if (ev.button.button != SDL_BUTTON_LEFT) {
                break;
            }

            // if not in MOUSE_BUTTON_STATE_DOWN then reset mouse_button_state and get out,
            // this is where MOUSE_BUTTON_STATE_MOTION state is exitted
            if (mouse_button_state != MOUSE_BUTTON_STATE_DOWN) {
                MOUSE_BUTTON_STATE_RESET;
                break;
            }
        
            // check for mouse_click event
            for (i = 0; i < SDL_EVENT_MAX; i++) {
                if (sdl_event_reg_tbl[i].type == SDL_EVENT_TYPE_MOUSE_CLICK &&
                    AT_POS(ev.button.x, ev.button.y, sdl_event_reg_tbl[i].pos, 0)) 
                {
                    event.event = i;
                    event.mouse_click.x = ev.button.x;
                    event.mouse_click.y = ev.button.y;
                    break;
                }
            }

            // reset mouse_button_state
            MOUSE_BUTTON_STATE_RESET;
            break; }

        case SDL_MOUSEMOTION: {
            // if MOUSE_BUTTON_STATE_NONE then get out
            if (mouse_button_state == MOUSE_BUTTON_STATE_NONE) {
                break;
            }

            // if in MOUSE_BUTTON_STATE_DOWN then check for mouse motion event; and if so
            // then set state to MOUSE_BUTTON_STATE_MOTION
            if (mouse_button_state == MOUSE_BUTTON_STATE_DOWN) {
                for (i = 0; i < SDL_EVENT_MAX; i++) {
                    if (sdl_event_reg_tbl[i].type == SDL_EVENT_TYPE_MOUSE_MOTION &&
                        AT_POS(ev.motion.x, ev.motion.y, sdl_event_reg_tbl[i].pos, 0)) 
                    {
                        mouse_button_state = MOUSE_BUTTON_STATE_MOTION;
                        mouse_button_motion_event = i;
                        break;
                    }
                }
            }

            // if did not find mouse_motion_event then reset mouse_button_state, and get out
            if (mouse_button_state != MOUSE_BUTTON_STATE_MOTION) {
                MOUSE_BUTTON_STATE_RESET;
                break;
            }

            // get all dditional pending mouse motion events, and sum the motion
            event.event = mouse_button_motion_event;
            event.mouse_motion.delta_x = 0;
            event.mouse_motion.delta_y = 0;
            do {
                event.mouse_motion.delta_x += ev.motion.x - mouse_button_x;
                event.mouse_motion.delta_y += ev.motion.y - mouse_button_y;
                mouse_button_x = ev.motion.x;
                mouse_button_y = ev.motion.y;
            } while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION) == 1);
            break; }

        case SDL_KEYDOWN: {
            int32_t  key = ev.key.keysym.sym;
            bool shift = (ev.key.keysym.mod & KMOD_SHIFT) != 0;

            if (!sdl_enable_keybd_events) {
                break;
            }

            if (key == 27) {
                event.event = SDL_EVENT_KEY_ESC;
            } else if (key == 8) {
                event.event = SDL_EVENT_KEY_BS;
            } else if (key == 9) {
                event.event = SDL_EVENT_KEY_TAB;
            } else if (key == 13) {
                event.event = SDL_EVENT_KEY_ENTER;
            } else if (!shift && ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9'))) {
                event.event = key;
            } else if (shift && (key >= 'a' && key <= 'z')) {
                event.event = 'A' + (key - 'a');
            } else if (shift && key == '-') {
                event.event = '_';
            } else if (key == ' ' || key == '.' || key == ',') {
                event.event = key;
            } else {
                break;
            }
            break; }

       case SDL_WINDOWEVENT: {
            DEBUG("got event SDL_WINOWEVENT - %s\n", SDL_WINDOWEVENT_STR(ev.window.event));
            switch (ev.window.event)  {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                sdl_win_width = ev.window.data1;
                sdl_win_height = ev.window.data2;
                event.event = SDL_EVENT_WIN_SIZE_CHANGE;
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                sdl_win_minimized = true;
                event.event = SDL_EVENT_WIN_MINIMIZED;
                break;
            case SDL_WINDOWEVENT_RESTORED:
                sdl_win_minimized = false;
                event.event = SDL_EVENT_WIN_RESTORED;
                break;
            }
            break; }

        case SDL_QUIT: {
            DEBUG("got event SDL_QUIT\n");
            sdl_quit = true;
            event.event = SDL_EVENT_QUIT;
            break; }

        default: {
            DEBUG("got event %d - not supported\n", ev.type);
            break; }
        }

        // break if event is set
        if (event.event != SDL_EVENT_NONE) {
            break; 
        }
    }

    return &event;
}

void sdl_play_event_sound(void)
{
    Mix_PlayChannel(-1, sdl_button_sound, 0);
}

// - - - - - - - - -  RENDER TEXT WITH EVENT HANDLING - - - - - - - - - - 

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
        sdl_event_register(event, SDL_EVENT_TYPE_TEXT, &pos);
    }
}

// - - - - - - - - -  RENDER RECTANGLES & CIRCLES - - - - - - - - - - - - 

// color to rgba
uint32_t sdl_pixel_rgba[] = {
    //    red           green          blue    alpha
       (127 << 24) |               (255 << 8) | 255,     // PURPLE
                                   (255 << 8) | 255,     // BLUE
                     (255 << 16) | (255 << 8) | 255,     // LIGHT_BLUE
                     (255 << 16)              | 255,     // GREEN
       (255 << 24) | (255 << 16)              | 255,     // YELLOW
       (255 << 24) | (128 << 16)              | 255,     // ORANGE
       (255 << 24)                            | 255,     // RED         
       (224 << 24) | (224 << 16) | (224 << 8) | 255,     // GRAY        
       (255 << 24) | (105 << 16) | (180 << 8) | 255,     // PINK 
       (255 << 24) | (255 << 16) | (255 << 8) | 255,     // WHITE       
                                        };

uint32_t sdl_pixel_r[] = {
       127,     // PURPLE
       0,       // BLUE
       0,       // LIGHT_BLUE
       0,       // GREEN
       255,     // YELLOW
       255,     // ORANGE
       255,     // RED         
       224,     // GRAY        
       255,     // PINK 
       255,     // WHITE       
            };

uint32_t sdl_pixel_g[] = {
       0,       // PURPLE
       0,       // BLUE
       255,     // LIGHT_BLUE
       255,     // GREEN
       255,     // YELLOW
       128,     // ORANGE
       0,       // RED         
       224,     // GRAY        
       105,     // PINK 
       255,     // WHITE       
            };

uint32_t sdl_pixel_b[] = {
       255,     // PURPLE
       255,     // BLUE
       255,     // LIGHT_BLUE
       0,       // GREEN
       0,       // YELLOW
       0,       // ORANGE
       0,       // RED         
       224,     // GRAY        
       180,     // PINK 
       255,     // WHITE       
            };


void sdl_render_rect(SDL_Rect * rect_arg, int32_t line_width, uint32_t rgba)
{
    SDL_Rect rect = *rect_arg;
    int32_t i;
    uint8_t r, g, b, a;

    // xxx endian
    r = (rgba >> 24) & 0xff;
    g = (rgba >> 16) & 0xff;
    b = (rgba >>  8) & 0xff;
    a = (rgba      ) & 0xff;

    SDL_SetRenderDrawColor(sdl_renderer, r, g, b, a);

    for (i = 0; i < line_width; i++) {
        SDL_RenderDrawRect(sdl_renderer, &rect);
        if (rect.w < 2 || rect.h < 2) {
            break;
        }
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
    }
}
SDL_Texture * sdl_create_filled_circle_texture(int32_t radius, uint32_t rgba)
{
    int32_t width = 2 * radius + 1;
    int32_t x = radius;
    int32_t y = 0;
    int32_t radiusError = 1-x;
    int32_t pixels[width][width];
    SDL_Texture * texture;

    #define DRAWLINE(Y, XS, XE, V) \
        do { \
            int32_t i; \
            for (i = XS; i <= XE; i++) { \
                pixels[Y][i] = (V); \
            } \
        } while (0)

    // initialize pixels
    bzero(pixels,sizeof(pixels));
    while(x >= y) {
        DRAWLINE(y+radius, -x+radius, x+radius, rgba);
        DRAWLINE(x+radius, -y+radius, y+radius, rgba);
        DRAWLINE(-y+radius, -x+radius, x+radius, rgba);
        DRAWLINE(-x+radius, -y+radius, y+radius, rgba);
        y++;
        if (radiusError<0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x) + 1;
        }
    }

    // create the texture and copy the pixels to the texture
    texture = SDL_CreateTexture(sdl_renderer,
                                SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STATIC,
                                width, width);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, NULL, pixels, width*4);

    // return texture
    return texture;
}

void sdl_render_circle(int32_t x, int32_t y, SDL_Texture * circle_texture)
{
    SDL_Rect rect;
    int32_t w, h;

    SDL_QueryTexture(circle_texture, NULL, NULL, &w, &h);

    rect.x = x - w/2;
    rect.y = y - h/2;
    rect.w = w;
    rect.h = h;

    SDL_RenderCopy(sdl_renderer, circle_texture, NULL, &rect);
}

// - - - - - - - - -  PREDEFINED DISPLAYS  - - - - - - - - - - - - - - - 

#define SDL_EVENT_BACK  (SDL_EVENT_USER_START+0)

void sdl_display_get_string(int32_t count, ...)
{
    char        * prompt_str[10];  //xxx why 10
    char        * curr_str[10];
    char        * ret_str[10];

    va_list       ap;
    SDL_Rect      keybdpane; 
    char          str[200];
    int32_t       i;
    sdl_event_t * event;

    int32_t       field_select;
    bool          shift;

    // this supports up to 4 fields
    if (count > 4) {
        ERROR("count %d too big, max 4\n", count);
    }

    // set sdl_enable_keybd_events, to enable keyboard input
    sdl_enable_keybd_events = true;

    // transfer variable arg list to array
    va_start(ap, count);
    for (i = 0; i < count; i++) {
        prompt_str[i] = va_arg(ap, char*);
        curr_str[i] = va_arg(ap, char*);
        ret_str[i] = va_arg(ap, char*);
    }
    va_end(ap);

    // init ret_str to curr_str
    for (i = 0; i < count; i++) {
        strcpy(ret_str[i], curr_str[i]);
    }

    // other init
    field_select = 0;
    shift = false;
    
    // loop until ENTER or ESC event received
    while (true) {
        // short sleep
        usleep(5000);

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
                                               "zxcvbnm,." };
        static char * row_chars_shift[4]   = { "1234567890_",
                                               "QWERTYUIOP",
                                               "ASDFGHJKL",
                                               "ZXCVBNM,." };
        char ** row_chars;
        int32_t r, c, i, j;

        row_chars = (!shift ? row_chars_unshift : row_chars_shift);
        r = SDL_PANE_ROWS(&keybdpane,1) / 2 - 10;
        if (r < 0) {
            r = 0;
        }
        c = (SDL_PANE_COLS(&keybdpane,1) - 33) / 2;

        for (i = 0; i < 4; i++) {
            for (j = 0; row_chars[i][j] != '\0'; j++) {
                char s[2];
                s[0] = row_chars[i][j];
                s[1] = '\0';
                sdl_render_text_ex(&keybdpane, r+2*i, c+3*j, s, s[0], 1, false, 1);
            }
        }
        sdl_render_text_ex(&keybdpane, r+6, c+27,  "SPACE",  ' ',   5, false, 1);

        sdl_render_text_ex(&keybdpane, r+8, c+0,  "SHIFT",   SDL_EVENT_KEY_SHIFT,   5, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+8,  "BS",      SDL_EVENT_KEY_BS,      2, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+13, "TAB",     SDL_EVENT_KEY_TAB,     3, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+19, "ESC",     SDL_EVENT_KEY_ESC,     3, false, 1);
        sdl_render_text_ex(&keybdpane, r+8, c+25, "ENTER",   SDL_EVENT_KEY_ENTER,   5, false, 1);

        // draw prompts
        for (i = 0; i < count; i++) {
            sprintf(str, "%s: %s", prompt_str[i], ret_str[i]);
            if ((i == field_select) && ((microsec_timer() % 1000000) > 500000) ) {
                strcat(str, "_");
            }
            sdl_render_text_ex(&keybdpane, 
                               r + 10 + 2 * (i % 2), 
                               i / 2 * SDL_PANE_COLS(&keybdpane,1) / 2, 
                               str, 
                               SDL_EVENT_FIELD_SELECT+i,
                               SDL_FIELD_COLS_UNLIMITTED, false, 1);
        }

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // handle events 
        event = sdl_poll_event();
        if (event->event == SDL_EVENT_QUIT) {
            sdl_play_event_sound();
            break;
        } else if (event->event == SDL_EVENT_KEY_SHIFT) {
            sdl_play_event_sound();
            shift = !shift;
        } else if (event->event == SDL_EVENT_KEY_BS) {
            int32_t len = strlen(ret_str[field_select]);
            sdl_play_event_sound();
            if (len > 0) {
                ret_str[field_select][len-1] = '\0';
            }
        } else if (event->event == SDL_EVENT_KEY_TAB) {
            sdl_play_event_sound();
            field_select = (field_select + 1) % count;
        } else if (event->event == SDL_EVENT_KEY_ENTER) {
            sdl_play_event_sound();
            break;
        } else if (event->event == SDL_EVENT_KEY_ESC) {
            sdl_play_event_sound();
            if (strcmp(ret_str[field_select], curr_str[field_select])) {
                strcpy(ret_str[field_select], curr_str[field_select]);
            } else {
                for (i = 0; i < count; i++) {
                    strcpy(ret_str[i], curr_str[i]);
                }
                break;
            }
        } else if (event->event >= SDL_EVENT_FIELD_SELECT && event->event < SDL_EVENT_FIELD_SELECT+count) {
            sdl_play_event_sound();
            field_select = event->event - SDL_EVENT_FIELD_SELECT;
        } else if (event->event >= 0x20 && event->event <= 0x7e) {
            char s[2];
            sdl_play_event_sound();
            s[0] = event->event;
            s[1] = '\0';
            strcat(ret_str[field_select], s);
        }
    }

    sdl_enable_keybd_events = false; 
}

void sdl_display_text(char * title, char **lines)
{
    SDL_Rect       title_line_pane;
    SDL_Rect       text_pane;
    int32_t        title_line_pane_height;
    sdl_event_t  * event;
    bool           done = false;

    while (!done) {
        // short delay
        usleep(5000);

        // init panes and event
        title_line_pane_height = sdl_font[0].char_height * 2;
        SDL_INIT_PANE(title_line_pane, 
                    0, 0,  
                    sdl_win_width, title_line_pane_height);
        SDL_INIT_PANE(text_pane, 
                    0, title_line_pane_height,
                    sdl_win_width, sdl_win_height - title_line_pane_height);
        sdl_event_init();

        // clear display, and init events
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // display title
        sdl_render_text_ex(&title_line_pane, 0, 0, title, SDL_EVENT_NONE,
                           SDL_FIELD_COLS_UNLIMITTED, true, 0);

        // display lines
        // XXX

        // display controls
        sdl_render_text_font0(&text_pane, 
                              SDL_PANE_ROWS(&text_pane,0)-1, SDL_PANE_COLS(&text_pane,0)-5, 
                              "BACK", SDL_EVENT_BACK);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // wait for event
        event = sdl_poll_event();
        switch (event->event) {
        case SDL_EVENT_BACK:
        case SDL_EVENT_QUIT: 
            sdl_play_event_sound();
            done = true;
        }
    }
}

void  sdl_display_choose_from_list(char * title_str, char ** choice, int32_t max_choice, int32_t * selection)
{
    SDL_Rect       title_line_pane;
    SDL_Rect       selection_pane;
    int32_t        title_line_pane_height, i;
    sdl_event_t  * event;

    // XXX needs scroll up/down

    // preset return
    *selection = -1;

    // loop until selection made, or aborted
    while (true) {
        // short delay
        usleep(5000);

        // this display has 2 panes
        // - title_line_pane: the top 2 lines
        // - selection_pane: the remainder
        title_line_pane_height = sdl_font[0].char_height * 2;
        SDL_INIT_PANE(title_line_pane, 
                      0, 0,  
                      sdl_win_width, title_line_pane_height);
        SDL_INIT_PANE(selection_pane, 
                      0, title_line_pane_height,
                      sdl_win_width, sdl_win_height - title_line_pane_height);
        sdl_event_init();

        // clear window
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // render the title line and selection lines, and 'back' button
        sdl_render_text_font0(&title_line_pane, 0, 0, title_str, SDL_EVENT_NONE);

        for (i = 0; i < max_choice; i++) {
            sdl_render_text_font0(&selection_pane, 2*i, 0, choice[i], SDL_EVENT_USER_START+i+1);
        }

        sdl_render_text_font0(&selection_pane, 
                              SDL_PANE_ROWS(&selection_pane,0)-1, SDL_PANE_COLS(&selection_pane,0)-5, 
                              "BACK", SDL_EVENT_BACK);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // handle events
        event = sdl_poll_event();
        if (event->event >= SDL_EVENT_USER_START+1 && event->event < SDL_EVENT_USER_START+max_choice+1) {
            sdl_play_event_sound();
            *selection = event->event - SDL_EVENT_USER_START - 1;
            break;
        } else if (event->event == SDL_EVENT_BACK || event->event == SDL_EVENT_QUIT) {
            sdl_play_event_sound();
            break;
        }
    }
}

// -----------------  PTHREAD ADDITIONS FOR ANDROID  ---------------------

#ifdef ANDROID

// Android NDK does not include pthread_barrier support, so add it here

int Pthread_barrier_init(Pthread_barrier_t *barrier,
    const Pthread_barrierattr_t *attr, unsigned count)
{
    if (count == 0) {
        FATAL("barrier count being set to 0\n");
    }

    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->cond, NULL);
    barrier->count = count;
    barrier->current = 0;
    return 0;
}

int Pthread_barrier_wait(Pthread_barrier_t *barrier)
{
    uint64_t done;

    if (barrier->count == 1) {
        return;
    }

    pthread_mutex_lock(&barrier->mutex);
    done = barrier->current / barrier->count + 1;
    barrier->current++;
    if ((barrier->current % barrier->count) == 0) {
        pthread_cond_broadcast(&barrier->cond);
    } else {
        do {
            pthread_cond_wait(&barrier->cond, &barrier->mutex);
        } while (barrier->current / barrier->count != done);
    }
    pthread_mutex_unlock(&barrier->mutex);
    return 0;
}

int Pthread_barrier_destroy(Pthread_barrier_t *barrier)
{
    // xxx should delete the mutex and cond
    return 0;
}

#endif

// -----------------  CONFIG READ / WRITE  -------------------------------

int config_read(char * filename, config_t * config)
{
    FILE * fp;
    int    i, version=0;
    char * name;
    char * value;
    char * saveptr;
    char   s[100] = "";
    const char * config_dir;
    char   config_path[PATH_MAX];

    // get directory for config filename, and 
    // construct config_path
#ifndef ANDROID
    config_dir = getenv("HOME");
#else
    config_dir = SDL_AndroidGetInternalStoragePath();
#endif
    if (config_dir == NULL) {
        ERROR("failed get config_dir\n");
        return -1;
    }
    sprintf(config_path, "%s/%s", config_dir, filename);

    // open config_file and verify version, 
    // if this fails then write the config file with default values
    if ((fp = fopen(config_path, "re")) == NULL ||
        fgets(s, sizeof(s), fp) == NULL ||
        sscanf(s, "VERSION %d", &version) != 1 ||
        version != config->version)
    {
        if (fp != NULL) {
            fclose(fp);
        }
        INFO("creating default config file %s, version=%d\n", config_path, config->version);
        return config_write(filename, config);
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

        for (i = 0; config->ent[i].name[0]; i++) {
            if (strcmp(name, config->ent[i].name) == 0) {
                strcpy(config->ent[i].value, value);
                break;
            }
        }
    }

    // close
    fclose(fp);
    return 0;
}

int config_write(char * filename, config_t * config)
{
    FILE * fp;
    int    i;
    const char * config_dir;
    char   config_path[PATH_MAX];

    // get directory for config filename, and 
    // construct config_path
#ifndef ANDROID
    config_dir = getenv("HOME");
#else
    config_dir = SDL_AndroidGetInternalStoragePath();
#endif
    if (config_dir == NULL) {
        ERROR("failed get config_dir\n");
        return -1;
    }
    sprintf(config_path, "%s/%s", config_dir, filename);

    // open
    fp = fopen(config_path, "we");  // mode: truncate-or-create, close-on-exec
    if (fp == NULL) {
        ERROR("failed to write config file %s, %s\n", config_path, strerror(errno));
        return -1;
    }

    // write version
    fprintf(fp, "VERSION %d\n", config->version);

    // write name/value pairs
    for (i = 0; config->ent[i].name[0]; i++) {
        fprintf(fp, "%-20s %s\n", config->ent[i].name, config->ent[i].value);
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

char * dur2str(char * str, int64_t duration)
{
    #define YEAR_SECS (365.25 * 86400)

    if (duration < YEAR_SECS) {
        int64_t days, hours, minutes;

        days = duration / 86400;
        duration -= days * 86400;
        hours = duration / 3600;
        duration -= hours * 3600;
        minutes = duration / 60;
        duration -= minutes * 60;

        sprintf(str, "%d %2.2d:%2.2d", (int32_t)days, (int32_t)hours, (int32_t)minutes);
    } else {
        sprintf(str, "%0.3lf YEARS", duration/YEAR_SECS);
    }

    return str;
}

// -----------------  FILE ACCESS  ----------------------------------------

// xxx this section needs work
typedef struct {
    char * buff;
    size_t offset;
} File_t;

static void list_local_files(char * location, int32_t * max, char *** pathnames);
static void list_cloud_files(char * location, int32_t * max, char *** pathnames);
static void list_files_sort(char ** pathnames, int32_t max);

void list_files(char * location, int32_t * max, char *** pathnames)
{
    if (location[strlen(location)-1] != '/') {
        ERROR("invalid location '%s'\n", location);
        *max = 0;
        *pathnames = NULL;
        return;
    }

    if (strncmp(location, "http://", 7) != 0) {
        list_local_files(location, max, pathnames);
    } else {
        list_cloud_files(location, max, pathnames);
    }

    list_files_sort(*pathnames, *max);
}

void list_files_free(int32_t max, char ** pathnames)
{
    int32_t i;
    for (i = 0; i < max; i++) {
        free(pathnames[i]);
    }
    free(pathnames);
}

void * open_file(char * pathname)
{
    #define MAX_BUFF  1000000 

    File_t * F = NULL;
    void   * buff;

    buff = calloc(1,MAX_BUFF);

    if (strncmp(pathname, "http://", 7) != 0) {
        SDL_RWops * rw;
        size_t      len;

        rw = SDL_RWFromFile(pathname, "r");
        if (rw == NULL) {
            ERROR("open %s, %s\n", pathname, SDL_GetError());
            goto error;
        }

        len = SDL_RWread(rw, buff, 1, MAX_BUFF-1);
        if (len == 0) {
            ERROR("read %s, len=%d\n", pathname, (int32_t)len);
            SDL_RWclose(rw);
            goto error;
        }

        SDL_RWclose(rw);
    } else {
        char   cmd[200];
        char   s[200];
        size_t offset;
        FILE * fp;

        // xxx check for overun of buff
        sprintf(cmd, "curl %s 2>/dev/null", pathname); 
        fp = popen(cmd, "r");
        if (fp == NULL) {
            ERROR("popen %s\n", cmd);
            goto error;
        }

        offset = 0;
        while (fgets(s, sizeof(s), fp) != NULL) {
            strcpy(buff+offset, s);
            offset += strlen(s);
        }

        fclose(fp);
    }

    F = calloc(1,sizeof(file_t));
    F->buff = buff;
    F->offset = 0;
    return F;

error:
    free(F);
    free(buff);
    return NULL;
}

char * read_file_line(file_t * f)
{
    File_t * F = f;
    char   * s;
    char   * tmp;

    if (F->buff[F->offset] == '\0') {
        return NULL;
    }

    s = &F->buff[F->offset];
    tmp = strchr(s, '\n');
    if (tmp != NULL) {
        *tmp = 0;
        F->offset = tmp - F->buff + 1;
    } else {
        F->offset = s + strlen(s) - F->buff;
    }

    return s;
}

void close_file(file_t * f)
{
    File_t * F = f;

    if (F == NULL) {
        return;
    }

    free(F->buff);
    free(F);
}

static void list_cloud_files(char * location, int32_t * max, char *** pathnames)
{
    FILE * fp;
    char * fn, * tmp;
    char   s[200];
    char   cmd[200];

    *max = 0;
    *pathnames = calloc(1000,sizeof(char*)); //xxx, maybe realloc

    sprintf(cmd, "curl %s 2>/dev/null", location);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        ERROR("popen %s\n", cmd);
        return;
    }

    while (fgets(s, sizeof(s), fp) != NULL) {
        if (strncmp(s, "<a href=\"", 9) == 0) {
            fn = s+9;
            tmp = strchr(fn, '\"');
            if (tmp == NULL) {
                ERROR("no closing quote\n");
                break;
            }
            *tmp = '\0';

            (*pathnames)[*max] = malloc(strlen(location)+strlen(fn)+1);
            sprintf((*pathnames)[*max], "%s%s", location, fn);
            (*max)++;
        }
    }

    fclose(fp);
}

#ifndef ANDROID
static void list_local_files(char * location, int32_t * max, char *** pathnames)
{
    DIR           * dir;
    struct dirent * dirent;
    int32_t         ret;
    struct stat     buf;
    char            path[300];

    *max = 0;
    *pathnames = calloc(1000,sizeof(char*)); //xxx

    dir = opendir(location);
    if (dir == NULL) {
        return;
    }

    while ((dirent = readdir(dir)) != NULL) {
        sprintf(path, "%s%s", location, dirent->d_name);
        ret = stat(path, &buf);
        if (ret != 0 || !S_ISREG(buf.st_mode)) {
            continue;
        }

        (*pathnames)[*max] = malloc(strlen(location)+strlen(dirent->d_name)+1);
        sprintf((*pathnames)[*max], "%s%s", location, dirent->d_name);
        (*max)++;
    }

    closedir(dir);
}
#else
JNIEnv* Android_JNI_GetEnv(void);
extern jclass mActivityClass;

static void list_local_files(char * location, int32_t * max, char *** pathnames)
{
    jmethodID       mid;
    jobject         context;
    jobject         java_asset_manager;
    AAssetManager * asset_manager;
    AAssetDir     * asset_dir;
    const char    * fn;
    JNIEnv        * mEnv = Android_JNI_GetEnv();
    char            location_copy[200];
    int32_t         len;

    // xxx comments
    // src/core/android/SDL_android.c

    *max = 0;
    *pathnames = calloc(1000,sizeof(char*)); //xxx

    // xxx 
    (*mEnv)->PushLocalFrame(mEnv, 16);

    // context = SDLActivity.getContext(); 
    mid = (*mEnv)->GetStaticMethodID(mEnv, mActivityClass,   // xxx mActivityClass
            "getContext","()Landroid/content/Context;");
    context = (*mEnv)->CallStaticObjectMethod(mEnv, mActivityClass, mid);

    // asset_manager = context.getAssets();
    mid = (*mEnv)->GetMethodID(mEnv, (*mEnv)->GetObjectClass(mEnv, context),
            "getAssets", "()Landroid/content/res/AssetManager;");
    java_asset_manager = (*mEnv)->CallObjectMethod(mEnv, context, mid);

    // xxx
    asset_manager = AAssetManager_fromJava(mEnv, java_asset_manager);

    // xxx
    strcpy(location_copy, location);
    len = strlen(location_copy);
    if (len > 0 && location_copy[len-1] == '/') {
        location_copy[len-1] = '\0';
    }

    // xxx
    asset_dir = AAssetManager_openDir(asset_manager, location_copy);
    while ((fn = AAssetDir_getNextFileName(asset_dir)) != NULL) {
        (*pathnames)[*max] = malloc(strlen(location_copy)+strlen(fn)+2);
        sprintf((*pathnames)[*max], "%s/%s", location_copy, fn);
        (*max)++;
    }

    // xxx
    AAssetDir_close(asset_dir);

    // xxx
    (*mEnv)->PopLocalFrame(mEnv, NULL);
}
#endif

static void list_files_sort(char ** pathnames, int32_t max)
{
    int start, i, min_idx;
    char * min_val;

    // selection sort ...

    // return if no sorting needed
    if (max <= 1) {
        return;
    }

    // search the pathnames for the minimum value, and swap the minimum value
    // with the value at the start of pathnames;  
    // increment start of pathnames and repeat
    for (start = 0; start < max-1; start++) {
        // find element in pathnames[start...] with minimum value
        min_val = pathnames[start];
        min_idx = start;
        for (i = start+1; i < max; i++) {
            if (strcmp(pathnames[i], min_val) < 0) {
                min_val = pathnames[i];
                min_idx = i;
            }
        }

        // swap element found with element at postion start
        char * tmp = pathnames[start];
        pathnames[start] = pathnames[min_idx];
        pathnames[min_idx] = tmp;
    }
}

// -----------------  MISC UTILS  -----------------------------------------

int32_t random_uniform(int32_t low, int32_t high)
{
    int32_t range = high - low + 1;

    // XXX this needs work
    return low + (random() % range);
}

// XXX use random?
// Refer to:
// - http://en.wikipedia.org/wiki/Triangular_distribution
// - http://stackoverflow.com/questions/3510475/generate-random-numbers-according-to-distributions
int32_t random_triangular(int32_t low, int32_t high)
{
    int32_t range = high - low;
    double U = rand() / (double) RAND_MAX;

    if (U <= 0.5) {
        return low + sqrt(U * range * range / 2);   
    } else {
        return high - sqrt((1 - U) * range * range / 2);  
    }
}

// Refer to:
// - http://scienceprimer.com/javascript-code-convert-light-wavelength-color
void wavelength_to_rgb(int32_t wl, uint8_t * r, uint8_t * g, uint8_t * b)
{
    // clip input wavelength to the allowed range
    if (wl < 380) {
        wl = 380;
    }
    if (wl > 780) {
        wl = 780;
    }

    // convert wavelength to r,g,b
    if (wl >= 380 && wl < 440) {
        *r = 255 * (440 - wl) / (440 - 380);
        *g = 0;
        *b = 255;
    } else if (wl >= 440 && wl < 490) {
        *r = 0;
        *g = (wl - 440) / (490 - 440);
        *b = 255;  
    } else if (wl >= 490 && wl < 510) {
        *r = 0;
        *g = 255;
        *b = 255 * (510 - wl) / (510 - 490);
    } else if (wl >= 510 && wl < 580) {
        *r = (wl - 510) / (580 - 510);
        *g = 255;
        *b = 0;
    } else if (wl >= 580 && wl < 645) {
        *r = 255;
        *g = 255 * (645 - wl) / (645 - 580);
        *b = 0;
    } else if (wl >= 645 && wl <= 780) {
        *r = 255;
        *g = 0;
        *b = 0;
    } else {
        *r = 0;
        *g = 0;
        *b = 0;
    }
}

