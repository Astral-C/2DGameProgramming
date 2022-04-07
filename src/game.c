#include <SDL.h>
#include <time.h>
#include "gf2d_graphics.h"
#include "gf2d_sprite.h"
#include "gfc_input.h"
#include "simple_logger.h"

#include "pg_entity.h"
#include "pg_ui.h"

#include "camera.h"
#include "player.h"
#include "enemy.h"
#include "npc.h"
#include "map.h"
#include "inventory.h"


typedef enum {
    PLAY,
    MENU,
    TEXTBOX //?
} gamestate;

int main(int argc, char * argv[])
{
    /*variable declarations*/
    int done = 0;
    const Uint8 * keys;


    gamestate state = PLAY;//MENU; //PLAY;

    float mf = 0;
    
    srand(time(0));
    
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
    inventory_init();

    gfc_input_init("input.json");

    load_enemy_configs();
        
    init_camera(350.0f, 200.0f);

    SDL_ShowCursor(SDL_DISABLE);
    
    Entity* p = player_new();
    entity_manager_set_player(p);
    set_camera_target(p);

    map_load("test_map.json");
    
    ui_manager_add_image(vector2d(16,16), "images/health.png", 24, 16, 1);

    /*
    Menu Manager Code
    */


    /*main game loop*/
    while(!done)
    {
        //int start = SDL_GetTicks();
        SDL_PumpEvents();   // update SDL's internal event structures
        keys = SDL_GetKeyboardState(NULL); // get the keyboard state for this frame
        /*update things here*/
        //SDL_GetMouseState(&mx,&my);
        mf+=0.1;
        if (mf >= 16.0)mf = 0;

        
        gf2d_graphics_clear_screen();// clears drawing buffers
        // all drawing should happen betweem clear_screen and next_frame
            //backgrounds drawn first
            
            
            switch (state)
            {
            case PLAY:

                if(gfc_input_key_released("d")){
                    entity_manager_toggle_draw_debug();
                }

                entity_manager_think_all();

                map_manager_draw_bg();
                entity_manager_draw_entities();
                map_manager_draw_fg();
                ui_manager_draw();

                inventory_show_consumables();
                player_draw_wallet();

                draw_shop_ui();

                update_camera();

                map_manager_update();
                break;

            case MENU:
                break;

            default:
                break;
            }


        gfc_input_update();
        gf2d_grahics_next_frame();// render current draw frame and skip to the next frame
        //slog("Ticks this frame %i", SDL_GetTicks() - start);
        
        if (keys[SDL_SCANCODE_ESCAPE])done = 1; // exit condition
        //printf("Rendering at %f FPS\r",gf2d_graphics_get_frames_per_second());
    }

    slog("---==== END ====---");
    return 0;
}
/*eol@eof*/
