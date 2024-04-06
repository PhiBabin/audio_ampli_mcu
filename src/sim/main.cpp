#include <SDL.h>
#include <stdio.h>


#include "sim/LCD_Driver.h"


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
        
        SDL_Event e;
        bool quit = false;
        while(!quit)
        {
            // Execute main loop of arduino
            loop();
            SDL_UpdateWindowSurface(window);
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                {
                    quit = true;
                }
            }
        }
    }
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}