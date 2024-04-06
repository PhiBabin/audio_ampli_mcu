#include <SDL.h>
#include <stdio.h>


#include "sim/LCD_Driver.h"
#include "sim/pio_encoder.h"


void setup();
void loop();


int main( int argc, char* args[] )
{
    //The window we'll be rendering to
    SDL_Window* window = NULL;
    
    //The surface contained by the window
    SDL_Surface* screenSurface = NULL;

    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
    }
    else
    {
        //Create window
        window = SDL_CreateWindow("Audio Ampli UI simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LCD_WIDTH, LCD_HEIGHT, SDL_WINDOW_SHOWN);
        if( window == NULL )
        {
            printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
            return -1;
        }
        //Get window surface
        screenSurface = SDL_GetWindowSurface(window);
        LCD_hook_sdl(screenSurface);

        //Fill the surface white
        SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
        
        //Update the surface
        SDL_UpdateWindowSurface(window);
        
        // Call arduino's setup
        setup();
        
        SDL_Event event;
        bool quit = false;

        constexpr int FPS = 60;
        constexpr int frameDelay = 1000 / FPS;
        uint32_t frameStart = 0;
        int frameTime = 0;
        while(!quit)
        {
            frameStart = SDL_GetTicks();
            // Execute main loop of arduino
            loop();
            SDL_UpdateWindowSurface(window);
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                    case SDL_QUIT:
                        quit = true;
                        break;
                    case SDL_KEYDOWN:
                        switch( event.key.keysym.sym ){
                            case SDLK_w:
                                decrement_encoder(20, 100);
                                break;
                            case SDLK_s:
                                increment_encoder(20, 100);
                                break;
                            case SDLK_a:
                                decrement_encoder(18, 3);
                                break;
                            case SDLK_d:
                                increment_encoder(18, 3);
                                break;
                            default:
                                break;
                        }
                        break;
                }
            }
            frameTime = SDL_GetTicks() - frameStart;
            if(frameDelay > frameTime)
            {
                SDL_Delay(frameDelay - frameTime);
            }
        }
    }
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}