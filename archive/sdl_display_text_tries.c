#if 0
// XXX 
// Keeping this for future reference. This approach does not work on the Android device.
// It gets the following error from call to SDL_GetWindowSurface 
// "No hardware accelerated renderers available"

void sdl_display_text(char * text)
{
    SDL_Surface  * screen = NULL;
    SDL_Surface  * text_surface = NULL;
    SDL_Rect       srcrect;
    SDL_Color      white = {255,255,255,255};
    SDL_Rect       disp_pane;
    SDL_Rect       disp_pane_last = {0,0,0,0};
    sdl_event_t  * event;
    uint32_t       black;
    int32_t        lines_per_display;
    int32_t        text_y = 0;
    bool           done = false;

    // rener the entire text to a surface, this TTF call supports wrapping of long
    // lines and the newline character
    text_surface = TTF_RenderText_Blended_Wrapped(sdl_font[2].font, text, white, sdl_win_width);
    if (text_surface == NULL) {
        ERROR("TTF_RenderText_Blended_Wrapped %s\n", SDL_GetError());
        goto done;
    }

    // without this the mouse-up event that is pending when this routine is called, is lost
    SDL_PumpEvents();

    // loop until done
    while (!done) {
        // short delay
        usleep(5000);

        // init disp_pane and event 
        SDL_INIT_PANE(disp_pane, 0, 0, sdl_win_width, sdl_win_height);
        sdl_event_init();

        // if window size has changed, or first call then get the window surface
        if (memcmp(&disp_pane, &disp_pane_last, sizeof(disp_pane)) != 0) {
            screen = SDL_GetWindowSurface(sdl_window);
            if (screen == NULL) {
                ERROR("SDL_GetWindowSurface, %s\n", SDL_GetError());
                goto done;
            }
            disp_pane_last = disp_pane;
        }

        // clear display
        black = SDL_MapRGBA(screen->format,0,0,0,0);
        SDL_FillRect(screen, NULL, black);

        // sanitize text_y
        if (text_y > text_surface->h - sdl_win_height) {
            text_y = text_surface->h - sdl_win_height;
        }
        if (text_y < 0) {
            text_y = 0;
        } 

        // display the text
        srcrect.x = 0;
        srcrect.y = text_y;
        srcrect.w = sdl_win_width;  
        srcrect.h = sdl_win_height;
        SDL_BlitSurface(text_surface, &srcrect, screen, NULL);

        // display controls 
        sdl_render_text_font0_to_surface(&disp_pane, 
                                         0, SDL_PANE_COLS(&disp_pane,0)-5, 
                                         "HOME", SDL_EVENT_HOME, screen);
        sdl_render_text_font0_to_surface(&disp_pane, 
                                         2, SDL_PANE_COLS(&disp_pane,0)-5, 
                                         "END", SDL_EVENT_END, screen);
        sdl_render_text_font0_to_surface(&disp_pane, 
                                         4, SDL_PANE_COLS(&disp_pane,0)-5, 
                                         "PGUP", SDL_EVENT_PGUP, screen);
        sdl_render_text_font0_to_surface(&disp_pane, 
                                         6, SDL_PANE_COLS(&disp_pane,0)-5, 
                                         "PGDN", SDL_EVENT_PGDN, screen);
        sdl_render_text_font0_to_surface(&disp_pane, 
                                         SDL_PANE_ROWS(&disp_pane,0)-1, SDL_PANE_COLS(&disp_pane,0)-5, 
                                         "BACK", SDL_EVENT_BACK, screen);
        sdl_event_register(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_TYPE_MOUSE_MOTION, &disp_pane);
        sdl_event_register(SDL_EVENT_MOUSE_WHEEL, SDL_EVENT_TYPE_MOUSE_WHEEL, &disp_pane);

        // present the display
        SDL_UpdateWindowSurface(sdl_window);

        // wait for event
        event = sdl_poll_event();
        switch (event->event) {
        case SDL_EVENT_MOUSE_MOTION:
            text_y -= event->mouse_motion.delta_y;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            text_y -= event->mouse_wheel.delta_y * 2 * sdl_font[2].char_height;
            break;
        case SDL_EVENT_HOME:
            sdl_play_event_sound();
            text_y = 0;
            break;
        case SDL_EVENT_END:
            sdl_play_event_sound();
            text_y = text_surface->h - sdl_win_height;
            break;
        case SDL_EVENT_PGUP:
            sdl_play_event_sound();
            lines_per_display = sdl_win_height / (sdl_font[2].char_height + 2);
            text_y -= (lines_per_display -2) * (sdl_font[2].char_height + 2) + 4;
            break;
        case SDL_EVENT_PGDN:
            sdl_play_event_sound();
            lines_per_display = sdl_win_height / (sdl_font[2].char_height + 2);
            text_y += (lines_per_display -2) * (sdl_font[2].char_height + 2) + 4;
            break;
        case SDL_EVENT_BACK:
        case SDL_EVENT_QUIT: 
            sdl_play_event_sound();
            done = true;
        }
    }

    // free
done:
    if (text_surface) {
        SDL_FreeSurface(text_surface);
        text_surface = NULL;
    }
}

// Keeping this for future reference. This approach supports displaying text of 
// up to 4096 pixels, which is the limit of the texture size on my system. The
// texture size limit may be hardware dependant.

void sdl_display_text(char * text)
{
    SDL_Surface  * surface = NULL;
    SDL_Texture  * texture = NULL;
    SDL_Rect       disp_pane;
    SDL_Rect       disp_pane_last = {0,0,0,0};
    SDL_Rect       dstrect;
    SDL_Rect       srcrect;
    SDL_Color      white = {255,255,255,255};
    SDL_Color      black = {0,0,0,255};
    SDL_Color      text_color;
    sdl_event_t  * event;
    int32_t        surface_y  = 0;
    bool           done       = false;
    bool           white_text = true;

    text_color = (white_text ? white : black);

    while (!done) {
        // short delay
        usleep(5000);

        // init disp_pane and event 
        SDL_INIT_PANE(disp_pane, 0, 0, sdl_win_width, sdl_win_height);
        sdl_event_init();

        // if window size has changed, or first call then
        // init surface and texture for the text 
        if (memcmp(&disp_pane, &disp_pane_last, sizeof(disp_pane)) != 0) {
            if (texture) {
                SDL_DestroyTexture(texture);
                texture = NULL;
            }
            if (surface) {
                SDL_FreeSurface(surface);
                surface = NULL;
            }

            surface = TTF_RenderText_Blended_Wrapped(sdl_font[2].font, text, text_color, disp_pane.w);
            if (surface == NULL) {
                ERROR("TTF_RenderText_Blended_Wrapped, %s\n", SDL_GetError());
                break;   
            }
            INFO("W H %d %d\n", surface->w, surface->h);

            texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);
            if (texture == NULL) {
                ERROR("SDL_CreateTextureFromSurface, %s\n", SDL_GetError());
                break;   
            }

            disp_pane_last = disp_pane;
            surface_y = 0;
        }

        // clear display
        if (white_text) {
            SDL_SetRenderDrawColor(sdl_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        } else {
            SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        }
        SDL_RenderClear(sdl_renderer);

        // sanitize surface_y, this is the location of the text that is displayed
        // at the top of the display
        if (surface->h - surface_y < disp_pane.h) {
            surface_y = surface->h - disp_pane.h;
        }
        if (surface_y < 0) {
            surface_y = 0;
        } 

        // display the text
        srcrect.x = 0;
        srcrect.y = surface_y;
        srcrect.w = surface->w;
        srcrect.h = surface->h - surface_y;

        dstrect.x = 0;
        dstrect.y = 0;
        dstrect.w = srcrect.w;
        dstrect.h = srcrect.h;

        SDL_RenderCopy(sdl_renderer, texture, &srcrect, &dstrect);

        // display controls 
        sdl_render_text_font0(&disp_pane, 
                              SDL_PANE_ROWS(&disp_pane,0)-1, SDL_PANE_COLS(&disp_pane,0)-5, 
                              "BACK", SDL_EVENT_BACK);
        sdl_event_register(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_TYPE_MOUSE_MOTION, &disp_pane);
        sdl_event_register(SDL_EVENT_MOUSE_WHEEL, SDL_EVENT_TYPE_MOUSE_WHEEL, &disp_pane);

        // present the display
        SDL_RenderPresent(sdl_renderer);

        // wait for event
        event = sdl_poll_event();
        switch (event->event) {
        case SDL_EVENT_MOUSE_MOTION:
            surface_y -= event->mouse_motion.delta_y;
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            surface_y -= event->mouse_wheel.delta_y * 2 * sdl_font[2].char_height;
            break;
        case SDL_EVENT_BACK:
        case SDL_EVENT_QUIT: 
            sdl_play_event_sound();
            done = true;
        }
    }

    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    if (surface) {
        SDL_FreeSurface(surface);
        surface = NULL;
    }
}
#endif

