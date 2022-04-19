#include <string.h>
#include "simple_json_array.h"
#include "gfc_input.h"
#include "gf2d_graphics.h"
#include "menu.h"
#include "player.h"
#include "enemy.h"
#include "inventory.h"
#include "npc.h"
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
 * 
 *      Also utilizing stb_image for loading images for ui elements (the SDL functions are overkill for this)
 */

static struct nk_context* ctx = NULL;

//todo: put these in a struct?
static int show_console = 0;
static int debug_menu_enabled = 0;
static int map_editor_enabled = 0;
static int is_paused = 0;
static int show_hitbox = 1;
static TextLine command_line = {0};
static TextLine resource_location = {0};

static const char* enemy_types[] = {
    "Bat",
    "Skull",
    "Golem",
    "Mushroom",
    "Magician"
};

static struct {
    int selected_page;
    TextLine page_text;
    TextLine quest_prompt;
    TextLine quest_progress;
    TextLine quest_id;
    int loaded_text;
} NpcEditor = {0};

ConfigData* EnemyConfigs;

static struct {
    Rect* selected;
} MapEditor = {0};

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

    struct nk_font_atlas* atlas;

    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    gfc_set_pre_input_hook(menu_input_begin);
    gfc_set_event_handler(menu_process_event);
    gfc_set_post_input_hook(menu_input_end);

    is_paused = 0;

    EnemyConfigs = get_enemy_configs();

    atexit(nk_sdl_shutdown);
}

struct nk_context* menu_get_nk_context(){
    return ctx;
}


void process_cmd(){
    int arg_i;
    char* cmd = strtok(command_line, " ");
    if(gfc_line_cmp(cmd, "debug") == 0){
        debug_menu_enabled = 1;
    }
    if(gfc_line_cmp(cmd, "editor") == 0){
        map_editor_enabled = 1;
    }
    if(gfc_line_cmp(cmd, "kill") == 0){
        cmd = strtok(NULL, " ");
        if(cmd != NULL && strcmp(cmd, "all") == 0){
            printf("Argument: %s\n", cmd);
            entity_manager_kill_enemies();
        }
    }
    if(gfc_line_cmp(cmd, "music")){
        cmd = strtok(NULL, " ");
        if(cmd == NULL) return;

        if(strcmp(cmd, "play")){
            //SDL_PauseAudioDevice(dev, 0);
        }
    }
    if(gfc_line_cmp(cmd, "give") == 0){
        cmd = strtok(NULL, " ");
        if(cmd != NULL && strcmp(cmd, "money") == 0){
            cmd = strtok(NULL, " ");
            if(cmd != NULL){
                arg_i = atoi(cmd);
                player_add_money(arg_i);
            }
        }
        if(cmd != NULL && strcmp(cmd, "potion") == 0){
            cmd = strtok(NULL, " ");
            if(cmd != NULL && strcmp(cmd, "stam") == 0){
                cmd = strtok(NULL, " ");
                if(cmd != NULL){
                    arg_i = atoi(cmd);
                    inventory_add_consumable(STAMINA, arg_i);
                }
            }

            if(cmd != NULL && strcmp(cmd, "speed") == 0){
                cmd = strtok(NULL, " ");
                if(cmd != NULL){
                    arg_i = atoi(cmd);
                    inventory_add_consumable(SPEED, arg_i);
                }
            }
        }
    }
}

void menu_update(int* gamestate){ //Because the menu system can change the state of the game it needs access to it
    if(ctx == NULL) return;

    if(gfc_input_command_down("toggle_console")){
        show_console = !show_console;
    }

    if(show_console){
        nk_begin(ctx, "Console", nk_rect(0, 0, 1200, 55), NK_WINDOW_NO_SCROLLBAR);
        nk_layout_row_begin(ctx, NK_DYNAMIC, 44, 2);
        nk_layout_row_push(ctx, 0.9f);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, command_line, sizeof(command_line)-1, nk_filter_default);
        nk_layout_row_push(ctx, 0.1f);
        if(nk_button_label(ctx, "run")){
            process_cmd();
            memset(command_line, 0, sizeof(command_line));
        }
        nk_layout_row_end(ctx);
        nk_end(ctx);
    }

    if(debug_menu_enabled){
        if (nk_begin(ctx, "Debug", nk_rect(50, 50, 230, 250), NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)){
            nk_layout_row_dynamic(ctx, 32, 2);

            if(nk_button_label(ctx, "Pause/Play")){
                is_paused  = !is_paused;
            }

            nk_checkbox_label(ctx, "Show Hitboxes", &show_hitbox);
            entity_manager_set_draw_debug(!show_hitbox);

        }
        nk_end(ctx);
    }

    if(map_editor_enabled){
        if (nk_begin(ctx, "Map Editor", nk_rect(50, 50, 230, 250), NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)){

            nk_layout_row_dynamic(ctx, 32, 2);

            if(*gamestate == 2){
                if(nk_button_label(ctx, "Add Collision Rect")){
                    
                }

                if(nk_button_label(ctx, "Delete Collision Rect")){
                     
                }
            }

            if(nk_button_label(ctx, "Edit Map")){
                if(*gamestate == 0 && *gamestate != 2){
                    *gamestate = 2;
                } else {
                    *gamestate = 0;
                }
            }

            if(nk_button_label(ctx, "Save Map")){
                //todo
            }

            nk_layout_row_dynamic(ctx, 32, 1);
            if(nk_button_label(ctx, "Toggle Hitboxes")){
                show_hitbox = !show_hitbox;
                entity_manager_set_draw_debug(show_hitbox);
            }
        }
        nk_end(ctx);
    
    }

    if(*gamestate == 2){ //edit mode ui
        Entity* current = entity_manager_get_selected();
        if(current != NULL){
            if(nk_begin(ctx, "Entity Inspector", nk_rect(100, 200, 350, 400), NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_TITLE|NK_WINDOW_MINIMIZABLE)){
                nk_layout_row_dynamic(ctx, 32, 1);

                switch (current->type)
                {
                case ENT_ENEMY: {
                    int health;
                    EnemyType e = *((EnemyType*)current->data);
                    switch (e)
                    {
                    case ENEMY_GOLEM: {
                        int charge_speed, charge_timer, range;

                        nk_label(ctx, "Golem Config", NK_TEXT_ALIGN_LEFT);
                        health = EnemyConfigs->GolemConfig.health;
                        charge_speed = EnemyConfigs->GolemConfig.charge_speed;
                        charge_timer = EnemyConfigs->GolemConfig.charge_timer;
                        range = EnemyConfigs->GolemConfig.range;

                        nk_property_int(ctx, "Health", 1, &health, 255, 1, 1);
                        nk_property_int(ctx, "Charge Speed", 1, &charge_speed, 255, 1, 1);
                        nk_property_int(ctx, "Charge Timer", 1, &charge_timer, 255, 1, 1);
                        nk_property_int(ctx, "Range", 1, &range, 65535, 1, 1);

                        EnemyConfigs->GolemConfig.health = (Uint8)health;
                        EnemyConfigs->GolemConfig.charge_speed = (Uint8)charge_speed;
                        EnemyConfigs->GolemConfig.charge_timer = (Uint8)charge_timer;
                        EnemyConfigs->GolemConfig.range = (Uint16)range;

                        if(nk_button_label(ctx, "Save Config")){
                            SJson* new_golem_conf = sj_object_new();
                            sj_object_insert(new_golem_conf, "health", sj_new_int(health));
                            sj_object_insert(new_golem_conf, "charge_speed", sj_new_int(charge_speed));
                            sj_object_insert(new_golem_conf, "charge_timer", sj_new_int(charge_timer));
                            sj_object_insert(new_golem_conf, "range", sj_new_int(range));
                            sj_save(new_golem_conf, "golem_def.json");
                        }
                        break;
                    }

                    case ENEMY_SKULL: {
                        nk_label(ctx, "SKULL Selected", NK_TEXT_ALIGN_CENTERED);
                        break;
                    }

                    case ENEMY_SPAWNER: {
                        int max, interval, amount;
                        EnemySpawnerProps* props = (EnemySpawnerProps*)current->data;
                        EnemyType prev = props->spawn_type;

                        max = props->max_entities;
                        amount = props->spawn_amount;
                        interval = props->spawn_interval;

                        nk_label(ctx, "Spawn Type", NK_TEXT_ALIGN_LEFT);
                        props->spawn_type = nk_combo(ctx, enemy_types, 2, props->spawn_type, 25, nk_vec2(100,100));
                        if(props->spawn_type != prev){
                            switch (props->spawn_type)
                            {
                            case ENEMY_BAT:
                                current->sprite = gf2d_sprite_load_image("images/bat_spawner.png");
                                break;
                            case ENEMY_SKULL:
                                current->sprite = gf2d_sprite_load_image("images/skull_spawner.png");
                                break;
                            default:
                                break;
                            }
                        }

                        nk_property_int(ctx, "Max Entities", 1, &max, 100, 1, 1);
                        nk_property_int(ctx, "Spawn Interval", 1, &amount, 255, 1, 1);
                        nk_property_int(ctx, "Spawn Amount", 1, &interval, 255, 1, 1);


                        props->max_entities = (Uint8)max;
                        props->spawn_amount = (Uint8)amount;
                        props->spawn_interval = (Uint8)interval;
                        
                        break;
                    }
                    default:
                        break;
                    }
                    break;
                }

                case ENT_NPC: {
                    NpcProps* props = (NpcProps*)current->data;
                    nk_label(ctx, "Text Pages", NK_TEXT_ALIGN_LEFT);
                    
                    for(int i = 0; i < props->textbox_pages; i++){
                        char* label =  (char*)sj_get_string_value(sj_array_get_nth(sj_object_get_value(props->npc_json, "pages"), i));
                        if(label == NULL){
                            continue;
                        }
                        if(nk_button_label(ctx,label)){
                            memcpy(NpcEditor.page_text, sj_get_string_value(sj_array_get_nth(sj_object_get_value(props->npc_json, "pages"), i)), sizeof(TextLine));
                            NpcEditor.selected_page = i;
                        }
                    }

                    //add page button
                    nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 3);
                    nk_layout_row_push(ctx, 0.2);
                    if(nk_button_label(ctx, "+")){
                        sj_array_append(sj_object_get_value(props->npc_json, "pages"), sj_new_str(NpcEditor.page_text));
                        props->textbox_pages++;
                    }
                    nk_layout_row_push(ctx, 0.2);
                    if(nk_button_label(ctx, "-")){
                        sj_array_delete_nth(sj_object_get_value(props->npc_json, "pages"), NpcEditor.selected_page);
                        props->textbox_pages--;
                    }
                    nk_layout_row_push(ctx, 0.6);
                    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.page_text, sizeof(TextLine), nk_filter_default);
                    nk_layout_row_end(ctx);
                    nk_layout_row_dynamic(ctx, 32, 1);

                    if(nk_button_label(ctx, "Set Text")){                
                        SJson* str = sj_new_str(NpcEditor.page_text);
                        sj_array_replace_nth(sj_object_get_value(props->npc_json, "pages"), NpcEditor.selected_page, str);
                    }

                    struct nk_image sprite = nk_image_ptr(current->sprite->texture);
                    nk_layout_row_static(ctx, current->sprite->frame_h, current->sprite->frame_w, 1);
                    if(nk_button_image(ctx, sprite)){
                        //i dunno, do some sort of fs select for filepaths from npc assets folder?
                    }
                    nk_layout_row_dynamic(ctx, 32, 1);

                    //TODO: Fix this
                    int has_quest = !props->has_quest;
                    nk_checkbox_label(ctx, "Has Quest", &has_quest);
                    props->has_quest = !has_quest;
                    if(props->has_quest){
                        nk_label(ctx, "Quest ID", NK_TEXT_ALIGN_LEFT);
                        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_id, sizeof(TextLine), nk_filter_default);

                        nk_label(ctx, "Prompt Text", NK_TEXT_ALIGN_LEFT);
                        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_prompt, sizeof(TextLine), nk_filter_default);

                        nk_label(ctx, "In Progress Msg Text", NK_TEXT_ALIGN_LEFT);
                        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_progress, sizeof(TextLine), nk_filter_default);
                    }

                    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, resource_location, sizeof(TextLine), nk_filter_default);
                    if(nk_button_label(ctx, "Save Npc")){
                        sj_save(props->npc_json, resource_location);
                    }
                    break;
                }

                default:
                    break;
                }
            }
            nk_end(ctx);
        }
    }
}

int game_paused(){
    return is_paused;
}

void menu_draw(){
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}