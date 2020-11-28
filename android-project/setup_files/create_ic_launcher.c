// build and run example:
//
// gcc -Wall -g `sdl2-config --cflags` -lSDL2 -lSDL2_ttf -lpng -o create_ic_launcher create_ic_launcher.c
// ./create_ic_launcher icon.png 512

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <SDL.h>
#include <SDL_ttf.h>

// #define PNG_DEBUG 3
#include <png.h>

// max,min values
#define MAX_HEIGHT 512
#define MAX_WIDTH  512
#define MIN_HEIGHT 42
#define MIN_WIDTH  42

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
#define BLACK      10
#define MAX_COLOR  11

uint sdl_pixel_rgba[] = {
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
       (  0 << 24) | (  0 << 16) | (  0 << 8) | 255,     // WHITE       
                                        };

// variables
SDL_Window   * sdl_window;
SDL_Renderer * sdl_renderer;

// prototypes
void draw_launcher(int width, int height);
void usage(char * progname);

TTF_Font *sdl_create_font(int font_ptsize);
void sdl_render_text(TTF_Font *font, int x, int y, int fg_color, int bg_color, char *str);
void sdl_set_color(int color);
void sdl_render_fill_rect(SDL_Rect *rect, int color);
void sdl_render_rect(SDL_Rect * rect_arg, int line_width, int color);
void sdl_render_circle(int x, int y, int r, int color);
void write_png_file(char* file_name, int width, int height, void * pixels, int pitch);

// -----------------  DRAW YOUR LAUNCHER HERE  -----------------------------

#define MAX_CIRCLE_INFO (sizeof(circle_info)/sizeof(circle_info[0]))

// x,y range 0-99
typedef struct {
    int32_t x;
    int32_t y;
    int32_t color;
} circle_info_t;

circle_info_t circle_info[] = {
    { 30, 25, GREEN      },
    { 82, 18, GREEN      },
    { 47, 53, GREEN      },
    { 15, 86, GREEN      },
    { 70, 90, GREEN      },
    { 15, 10, YELLOW     },
    { 80, 60, RED        },
    { 40, 80, LIGHT_BLUE },
    { 65, 35, BLUE       },
    { 10, 40, PURPLE     },
    { 55, 15, RED        },
    { 22, 62, YELLOW     },
            };

void draw_launcher(int32_t width, int32_t height)
{
    SDL_Rect      rect;
    int32_t       i, x, y, r, color;

    // set circle radius
    r = 2 * width / 48;

    // draw cicles
    for (i = 0; i < MAX_CIRCLE_INFO; i++) {
        x     = circle_info[i].x * width / 100;
        y     = circle_info[i].y * height / 100;
        color = circle_info[i].color;

        sdl_render_circle(x, y, r, color);
    }

    // draw perimeter rect, with 2 pixel line width
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    sdl_render_rect(&rect, 2, GREEN);
}

// -----------------  MAIN  ------------------------------------------------

int main(int argc, char ** argv) 
{
    char   * filename;
    int      ret, width, height, icon_size;
    SDL_Rect rect;
    int      pixels[MAX_HEIGHT][MAX_WIDTH];

    // parse args: <filename> <icon_size>
    if (argc != 3) {
        usage(argv[0]);
    }
    filename = argv[1];
    if (sscanf(argv[2], "%d", &icon_size) != 1) {
        usage(argv[0]);
    }

    // initialize Simple DirectMedia Layer  (SDL)
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL_Init failed\n");
        exit(1);
    }

    // create SDL Window and Renderer
    if (SDL_CreateWindowAndRenderer(MAX_WIDTH, MAX_HEIGHT, 0, &sdl_window, &sdl_renderer) != 0) {
        printf("SDL_CreateWindowAndRenderer failed\n");
        exit(1);
    }

    // clear display
    SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(sdl_renderer);

    // draw_launcher
    width = height = icon_size;
    draw_launcher(width, height);

    // present the display
    SDL_RenderPresent(sdl_renderer);

    // delay so the disaplay can be seen briefly
    sleep(2);

    // read the pixels
    rect.x = 0;
    rect.y = 0; 
    rect.w = width;
    rect.h = height;
    ret = SDL_RenderReadPixels(sdl_renderer, &rect, SDL_PIXELFORMAT_ABGR8888, pixels, sizeof(pixels[0]));
    if (ret < 0) {
        printf("ERROR SDL_RenderReadPixels, %s\n", SDL_GetError());
        exit(1);
    }

    // write the png file
    write_png_file(filename, width, height, pixels, sizeof(pixels[0]));

    // cleanup
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 0;
}

void usage(char * progname) 
{
    printf("Usage: %s <filename> <icon_size>\n", 
           basename(strdup(progname)));
    exit(1);
}

// -----------------  SUPPORT ROUTINES  -------------------------------------------------

TTF_Font *sdl_create_font(int font_ptsize)
{
    static bool first_call = true;
    static char *font_path;
    TTF_Font *font;

    if (first_call) {
        #define MAX_FONT_SEARCH_PATH 2
        static char font_search_path[MAX_FONT_SEARCH_PATH][PATH_MAX];
        int i;

        if (TTF_Init() < 0) {
            printf("TTF_Init failed\n");
            exit(1);  
        }
        sprintf(font_search_path[0], "%s", "/usr/share/fonts/gnu-free/FreeMonoBold.ttf");
        sprintf(font_search_path[1], "%s", "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf");
        for (i = 0; i < MAX_FONT_SEARCH_PATH; i++) {
            struct stat buf;
            font_path = font_search_path[i];
            if (stat(font_path, &buf) == 0) {
                break;
            }
        }
        if (i == MAX_FONT_SEARCH_PATH) {
            printf("failed to locate font file\n");
            exit(1);  
        }
        first_call = false;
    }

    font = TTF_OpenFont(font_path, font_ptsize);
    if (font == NULL) {
        printf("failed TTF_OpenFont(%s)\n", font_path);
    }

    return font;
}

void sdl_render_text(TTF_Font *font, int x, int y, int fg_color, int bg_color, char *str)
{
    int          w,h;
    unsigned int fg_rgba, bg_rgba;
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Color    fg_sdl_color;
    SDL_Color    bg_sdl_color;

    fg_rgba = sdl_pixel_rgba[fg_color];
    fg_sdl_color.r = (fg_rgba >> 24) & 0xff;
    fg_sdl_color.g = (fg_rgba >> 16) & 0xff;
    fg_sdl_color.b = (fg_rgba >>  8) & 0xff;
    fg_sdl_color.a = (fg_rgba >>  0) & 0xff;

    bg_rgba = sdl_pixel_rgba[bg_color];
    bg_sdl_color.r = (bg_rgba >> 24) & 0xff;
    bg_sdl_color.g = (bg_rgba >> 16) & 0xff;
    bg_sdl_color.b = (bg_rgba >>  8) & 0xff;
    bg_sdl_color.a = (bg_rgba >>  0) & 0xff;

    surface = TTF_RenderText_Shaded(font, str, fg_sdl_color, bg_sdl_color);
    texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);

    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect dst = {x,y,w,h};
    SDL_RenderCopy(sdl_renderer, texture, NULL, &dst);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void sdl_set_color(int color)
{
    unsigned int rgba = sdl_pixel_rgba[color];
    unsigned int r = (rgba >> 24) & 0xff;
    unsigned int g = (rgba >> 16) & 0xff;
    unsigned int b = (rgba >>  8) & 0xff;
    unsigned int a = (rgba      ) & 0xff;

    SDL_SetRenderDrawColor(sdl_renderer, r, g, b, a);
}

void sdl_render_fill_rect(SDL_Rect *rect, int color)
{
    sdl_set_color(color);

    SDL_RenderFillRect(sdl_renderer, rect);
}

void sdl_render_rect(SDL_Rect * rect_arg, int line_width, int color)
{
    SDL_Rect rect = *rect_arg;
    int i;

    sdl_set_color(color);

    for (i = 0; i < line_width; i++) {
        SDL_RenderDrawRect(sdl_renderer, &rect);
        SDL_RenderDrawPoint(sdl_renderer, rect.x+rect.w-1, rect.y+rect.h-1);  // xxx workaround
        if (rect.w < 2 || rect.h < 2) {
            break;
        }
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
    }
}

void sdl_render_circle(int x_ctr, int y_ctr, int radius, int color)
{
    #define DRAWLINE(Y, XS, XE) \
        do { \
            SDL_RenderDrawLine(sdl_renderer, (XS)+x_ctr, (Y)+y_ctr,  (XE)+x_ctr, (Y)+y_ctr); \
        } while (0)

    int x = radius;
    int y = 0;
    int radiusError = 1-x;

    sdl_set_color(color);

    while(x >= y) {
        DRAWLINE(y, -x, x);
        DRAWLINE(x, -y, y);
        DRAWLINE(-y, -x, x);
        DRAWLINE(-x, -y, y);
        y++;
        if (radiusError<0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x) + 1;
        }
    }
}

void write_png_file(char* file_name, int width, int height, void * pixels, int pitch)
{
    png_byte  * row_pointers[height];
    FILE      * fp;
    png_structp png_ptr;
    png_infop   info_ptr;
    png_byte    color_type, bit_depth;
    int         y;

    // init
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    bit_depth = 8;
    for (y = 0; y < height; y++) {
        row_pointers[y] = pixels + (y * pitch);
    }

    // create file 
    fp = fopen(file_name, "wb");
    if (!fp) {
        printf("ERROR fopen %s\n", file_name);
        exit(1);
    }

    // initialize stuff 
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        printf("ERROR png_create_write_struct\n");
        exit(1);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        printf("ERROR png_create_info_struct\n");
        exit(1);
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("ERROR init_io failed\n");
        exit(1);
    }
    png_init_io(png_ptr, fp);

    // write header 
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("ERROR writing header\n");
        exit(1);
    }
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    // write bytes 
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("ERROR writing bytes\n");
        exit(1);
    }
    png_write_image(png_ptr, row_pointers);

    // end write 
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("ERROR end of write\n");
        exit(1);
    }
    png_write_end(png_ptr, NULL);

    // close fp
    fclose(fp);
}

