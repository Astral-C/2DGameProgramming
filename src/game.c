#include <SDL.h>
#include "gf2d_graphics.h"
#include "gf2d_sprite.h"
#include "simple_logger.h"

#include "pg_entity.h"
#include "pg_ui.h"

#include "player.h"
#include "map.h"

int main(int argc, char * argv[])
{
    /*variable declarations*/
    int done = 0;
    const Uint8 * keys;
    Sprite *sprite;
    Entity* ent;

    int mx,my;
    float mf = 0;
    Sprite *mouse;
    Vector4D mouseColor = {255,100,255,200};
    
    /*program initializtion*/
    init_logger("gf2d.log");
    slog("---==== BEGIN ====---");
    gf2d_graphics_initialize(
        "gf2d",
        1200,
        720,
        1200,
        720,
        vector4d(0,0,0,255),
        0);
    gf2d_graphics_set_frame_delay(16);
    gf2d_sprite_init(1024);
    entity_manager_init(1024);
    ui_manager_init_default(50);
    
    map_manager_init(5); //5 is a good number, figure you can swap between 3 main + 2 to each side that can load their own maps
    
    SDL_ShowCursor(SDL_DISABLE);
    

    /*demo setup*/
    mouse = gf2d_sprite_load_all("images/pointer.png",32,32,16);
    
    player_new();
    Map* test = map_load("test_map.json");
    Vector2D cam_pos, cam_zoom;

    cam_zoom = vector2d(4,4); 

    //ui_manager_add_text(vector2d(35*4,16), "the FUNNY funny");
    ui_manager_add_image(vector2d(16,16), "images/health.png", 24, 16, 1);

    /*main game loop*/
    while(!done)
    {
        SDL_PumpEvents();   // update SDL's internal event structures
        keys = SDL_GetKeyboardState(NULL); // get the keyboard state for this frame
        /*update things here*/
        SDL_GetMouseState(&mx,&my);
        mf+=0.1;
        if (mf >= 16.0)mf = 0;

        gf2d_graphics_clear_screen();// clears drawing buffers
        // all drawing should happen betweem clear_screen and next_frame
            //backgrounds drawn first
            
            //UI elements last
            
            entity_manager_think_all();
            
            map_manager_draw_bg(cam_pos, cam_zoom);

            entity_manager_draw_entities();
            
            map_manager_draw_fg(cam_pos, cam_zoom);

            ui_manager_draw();

            //TODO: add UI manager draw
            // ALSO cleanup ui manager and map manager properly

        gf2d_grahics_next_frame();// render current draw frame and skip to the next frame
        
        if (keys[SDL_SCANCODE_ESCAPE])done = 1; // exit condition
        printf("Rendering at %f FPS\r",gf2d_graphics_get_frames_per_second());
    }

    
    slog("---==== END ====---");
    return 0;
}
/*eol@eof*/
