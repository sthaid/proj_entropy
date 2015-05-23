/*
Copyright (c) 2015 Steven Haid

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

#include <SDL.h>
// #define PNG_DEBUG 3
#include <png.h>

// max,min values
#define MAX_HEIGHT 1000
#define MAX_WIDTH  1000
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
#define MAX_COLOR  10

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

// variables
SDL_Window   * sdl_window;
SDL_Renderer * sdl_renderer;

// prototypes
void draw_launcher(int32_t width, int32_t height);
void usage(char * progname);
void sdl_render_rect(SDL_Rect * rect_arg, int32_t line_width, uint32_t rgba);
SDL_Texture * sdl_create_filled_circle_texture(int32_t radius, uint32_t rgba);
void sdl_render_circle(int32_t x, int32_t y, SDL_Texture * circle_texture);
void write_png_file(char* file_name, int32_t width, int32_t height, void * pixels, int32_t pitch);

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
    SDL_Texture * texture;
    int32_t       i, x, y, r, color;

    // set circle radius
    r = 2 * width / 48;

    // draw cicles
    for (i = 0; i < MAX_CIRCLE_INFO; i++) {
        x     = circle_info[i].x * width / 100;
        y     = circle_info[i].y * height / 100;
        color = circle_info[i].color;

        texture =  sdl_create_filled_circle_texture(r, sdl_pixel_rgba[color]);
        sdl_render_circle(x, y, texture);
        SDL_DestroyTexture(texture);
    }

    // draw perimeter rect, with 2 pixel line width
    rect.x = 0;
    rect.y = 0; 
    rect.w = width;
    rect.h = height;
    sdl_render_rect(&rect, 2, sdl_pixel_rgba[GREEN]);
}

// -----------------  MAIN  ------------------------------------------------

int main(int argc, char ** argv) 
{
    char   * base_filename;
    int32_t  width[100], height[100];
    int32_t  ret, i, max_image=0;
    char     full_filename[1000];
    SDL_Rect rect;

    static int32_t pixels[MAX_HEIGHT][MAX_WIDTH];

    // parse args: <base_filename> <w,h> <w,h> ...
    if (argc < 3) {
        usage(argv[0]);
    }
    base_filename = argv[1];
    for (i = 2; i < argc; i++) {
        if (sscanf(argv[i], "%d,%d", &width[max_image], &height[max_image]) != 2 ||
            width[max_image] < MIN_WIDTH || width[max_image] > MAX_WIDTH ||
            height[max_image] < MIN_HEIGHT || height[max_image] > MAX_HEIGHT)
        {
            usage(argv[0]);
        }
        printf("%d: %d %d\n",i, width[max_image], height[max_image]);
        max_image++;
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

    // loop over desired image sizes
    for (i = 0; i < max_image; i++) {
        // clear display
        SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(sdl_renderer);

        // draw_launcher
        draw_launcher(width[i], height[i]);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // delay so the disaplay can be seen briefly
        sleep(1);

        // read the pixels
        rect.x = 0;
        rect.y = 0; 
        rect.w = width[i];
        rect.h = height[i];
        ret = SDL_RenderReadPixels(sdl_renderer, &rect, SDL_PIXELFORMAT_ABGR8888, pixels, sizeof(pixels[0]));
        if (ret < 0) {
            printf("ERROR SDL_RenderReadPixels, %s\n", SDL_GetError());
            exit(1);
        }

        // write the png file
        sprintf(full_filename, "%s_%d_%d.png", base_filename, width[i], height[i]);
        write_png_file(full_filename, width[i], height[i], pixels, sizeof(pixels[0]));
    }

    // cleanup
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 0;
}

void usage(char * progname) 
{
    printf("Usage: %s <base_filename> <w,h> <w,h> ...\n",
           basename(strdup(progname)));
    exit(1);
}

// -----------------  SUPPORT ROUTINES  -------------------------------------------------

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

void write_png_file(char* file_name, int32_t width, int32_t height, void * pixels, int32_t pitch)
{
    png_byte  * row_pointers[height];
    FILE      * fp;
    png_structp png_ptr;
    png_infop   info_ptr;
    png_byte    color_type, bit_depth;
    int32_t     y;

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

