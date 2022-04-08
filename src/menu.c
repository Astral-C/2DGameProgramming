#include <string.h>
#include "gfc_input.h"
#include "gf2d_graphics.h"
#include "pg_entity.h"
#include "menu.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

/*
 *      Menu System, uses Nuklear as a base
 *      https://github.com/Immediate-Mode-UI/Nuklear
 */

static struct nk_context* ctx = NULL;
static int show_console = 0;
static int debug_menu_enabled = 0;
static int is_paused = 0;
static int show_hitbox = 1;
static TextLine command_line = {0};

void menu_input_begin(){
    nk_input_begin(ctx);
}

void menu_process_event(SDL_Event* event){
    nk_sdl_handle_event(event);
}

void menu_input_end(){
    nk_input_end(ctx);
}

void init_menu(){
    ctx = nk_sdl_init(gf2d_graphics_get_window(), gf2d_graphics_get_renderer());

    struct nk_font_atlas atlas;

    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    gfc_set_pre_input_hook(menu_input_begin);
    gfc_set_event_handler(menu_process_event);
    gfc_set_post_input_hook(menu_input_end);

    is_paused = 0;

    atexit(nk_sdl_shutdown);
}

struct nk_context* menu_get_nk_context(){
    return ctx;
}

void menu_update(){
    if(ctx == NULL) return;
    
    if(gfc_input_command_down("toggle_console")){
        show_console = !show_console;
    }

    if(show_console){
        nk_begin(ctx, "Console", nk_rect(0, 0, 1200, 60), 0);
        nk_layout_row_begin(ctx, NK_DYNAMIC, 44, 2);
        nk_layout_row_push(ctx, 0.9f);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, command_line, sizeof(command_line)-1, nk_filter_default);
        nk_layout_row_push(ctx, 0.1f);
        if(nk_button_label(ctx, "run")){
            memset(command_line, 0, sizeof(command_line));
            if(gfc_line_cmp(command_line, "debug") == 0){
                debug_menu_enabled = 1;
            }
        }
        nk_layout_row_end(ctx);
        nk_end(ctx);
    }

    if(debug_menu_enabled){
        if (nk_begin(ctx, "Debug", nk_rect(50, 50, 230, 250), NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)){
            nk_layout_row_static(ctx,30,210,1);

            if(nk_button_label(ctx, "Pause/Play")){
                is_paused  = !is_paused;
            }
        
            nk_checkbox_label(ctx, "Show Hitboxes", &show_hitbox);
            entity_manager_set_draw_debug(!show_hitbox);
        }
        nk_end(ctx);
    }


}

int game_paused(){
    return is_paused;
}

void menu_draw(){
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}