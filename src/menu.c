#include <string.h>
#include "simple_json_array.h"
#include "gfc_input.h"
#include "gf2d_graphics.h"
#include "menu.h"
#include "player.h"
#include "enemy.h"
#include "inventory.h"
#include "npc.h"
#include "audio.h"
#include "camera.h"
#include "map.h"
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
static Entity* prev_selected = NULL;

static Sprite* start_bg = NULL;

static const char* enemy_types[] = {
    "Bat",
    "Skull",
    "Golem",
    "Mushroom",
    "Magician"
};

static const char* quest_types[] = {
    "Kill Enemies", 
    "Enable Flag"
};

static const char* consumable_types[] = {
    "Health",
    "Speed",
    "Stamina",
    "Invisibility",
    "Time Freeze"
};

static struct {
    int selected_page;
    TextLine page_text;
    TextLine quest_prompt;
    TextLine quest_progress;
    TextLine quest_id;
    int quest_type;
    int quest_tag;
    int completion_progress;
    int quest_loaded;
} NpcEditor = {0};

ConfigData* EnemyConfigs;

static struct {
    TextLine bg_path;
    TextLine fg_path;
    TextLine deco_path;

    Rect* selected;
    Rect new;
} MapEditor = {0};

void map_editor_notify_selected(Rect* rect){
    MapEditor.selected = rect;
}

void map_editor_notify_load(char* bg, char* fg, char* deco){
    if(bg != NULL){
        gfc_line_cpy(MapEditor.bg_path, bg);
    }
    if(fg != NULL){
        gfc_line_cpy(MapEditor.fg_path, fg);
    }
    if(deco != NULL){
        gfc_line_cpy(MapEditor.deco_path, deco);
    }
}

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

    start_bg = gf2d_sprite_load_image("images/main_menu.png");

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
    if(gfc_line_cmp(cmd, "music") == 0){
        cmd = strtok(NULL, " ");
        if(cmd == NULL) return;

        if(strcmp(cmd, "play") == 0){
            audio_play_mod();
        }
        if(strcmp(cmd, "pause") == 0){
            audio_pause_mod();
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
    

    if(gfc_input_command_released("pause") && *gamestate != 1){
        is_paused = 1;
        audio_pause_mod();
    }

    if(is_paused){
        nk_style_push_color(ctx, &ctx->style.window.background, nk_rgba(0,0,0,100));
        nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2(0,0));
        nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,100)));
        nk_begin(ctx, "pm_bg", nk_rect(0, 0, 1200, 720), 0);
        nk_end(ctx);
        nk_style_pop_vec2(ctx);
        
        nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2(50,25));
        nk_begin(ctx, "pause_menu", nk_rect(425, 260, 350, 200), 0);
        nk_layout_row_static(ctx, 32, 240, 1);
        if(nk_button_label(ctx, "Continue")){
            is_paused = 0;
            map_editor_enabled = 0;
            *gamestate = 0;
            map_manager_notify_editing(0);
            set_camera_target(entity_manager_get_player());
            audio_play_mod();
        }
        if(nk_button_label(ctx, "Save")){
            //SJson* save = sj_new();
            //sj_object_insert(save, "map", MapEd);
            //sj_save(save, "save.json");   
        }
        nk_layout_row_static(ctx, 32, 240, 1);
        if(nk_button_label(ctx, "Edit Map")){
            *gamestate = 2;
            map_editor_enabled = 1;
            is_paused = 0;
            map_manager_notify_editing(1);
            set_camera_target(NULL);
        }
        nk_layout_row_static(ctx, 32, 240, 1);
        if(nk_button_label(ctx, "Quit")){
            *gamestate = 3;
        }
        nk_end(ctx);

        nk_style_pop_vec2(ctx);
        nk_style_pop_color(ctx);
        nk_style_pop_style_item(ctx);
        return;
    }

    if(gfc_input_command_down("toggle_console")){
        show_console = !show_console;
    }

    if(show_console){
        nk_style_push_color(ctx, &ctx->style.window.background, nk_rgba(0,0,0,0));
        nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,0)));
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
        nk_style_pop_color(ctx);
        nk_style_pop_style_item(ctx);
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
                    map_editor_notify_add(MapEditor.new);
                }

                if(nk_button_label(ctx, "Delete Collision Rect")){
                    map_editor_notify_delete(MapEditor.selected);
                }

                if(MapEditor.selected != NULL){
                    nk_property_float(ctx, "x", 0, &MapEditor.selected->pos.x, current_map_width(), 16, 16);
                    nk_property_float(ctx, "y", 0, &MapEditor.selected->pos.y, current_map_height(), 16, 16);
                    nk_property_float(ctx, "w", 0, &MapEditor.selected->size.x, current_map_width(), 16, 16);
                    nk_property_float(ctx, "h", 0, &MapEditor.selected->size.y, current_map_height(), 16, 16);
                } else {
                    nk_property_float(ctx, "x", 0, &MapEditor.new.pos.x, current_map_width(), 16, 16);
                    nk_property_float(ctx, "y", 0, &MapEditor.new.pos.y, current_map_height(), 16, 16);
                    nk_property_float(ctx, "w", 0, &MapEditor.new.size.x, current_map_width(), 16, 16);
                    nk_property_float(ctx, "h", 0, &MapEditor.new.size.y, current_map_height(), 16, 16);
                }

            }

            nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);
            
            nk_layout_row_push(ctx, 0.2);
            nk_label(ctx, "BG Path", NK_TEXT_ALIGN_CENTERED);
            nk_layout_row_push(ctx, 0.8);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.bg_path, sizeof(TextLine), nk_filter_default);
            nk_layout_row_end(ctx);
            
            nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);
            nk_layout_row_push(ctx, 0.2);
            nk_label(ctx, "FG Path", NK_TEXT_ALIGN_CENTERED);
            nk_layout_row_push(ctx, 0.8);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.fg_path, sizeof(TextLine), nk_filter_default);
            nk_layout_row_end(ctx);
            
            nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);
            nk_layout_row_push(ctx, 0.2);
            nk_label(ctx, "Deco Path", NK_TEXT_ALIGN_CENTERED);
            nk_layout_row_push(ctx, 0.8);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.deco_path, sizeof(TextLine), nk_filter_default);
            nk_layout_row_end(ctx);

            nk_layout_row_dynamic(ctx, 32, 1);
            if(nk_button_label(ctx, "Deselect")){
                map_editor_notify_selected(NULL);
            }

            if(nk_button_label(ctx, "New Map")){
                map_editor_notify_selected(NULL);
                map_cleanup();
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
            if(current != prev_selected){
                NpcEditor.quest_loaded = 0;
                prev_selected = current;
            }

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
                    if(props->has_quest && NpcEditor.quest_loaded == 0){
                        SJson* quest = sj_object_get_value(props->npc_json, "quest");
                        if(quest != NULL){
                            gfc_line_cpy(NpcEditor.quest_id, sj_get_string_value(sj_object_get_value(quest, "id")));
                            gfc_line_cpy(NpcEditor.quest_prompt, sj_get_string_value(sj_object_get_value(quest, "incomplete_text")));
                            gfc_line_cpy(NpcEditor.quest_progress, sj_get_string_value(sj_object_get_value(quest, "progress_msg")));
                            sj_get_integer_value(sj_object_get_value(quest, "type"), &NpcEditor.quest_type);
                            sj_get_integer_value(sj_object_get_value(quest, "tag"), &NpcEditor.quest_tag);
                            sj_get_integer_value(sj_object_get_value(quest, "complete"), &NpcEditor.completion_progress);
                        }
                        NpcEditor.quest_loaded = 1;
                    }

                    int has_quest = !props->has_quest;
                    nk_checkbox_label(ctx, "Has Quest", &has_quest);
                    props->has_quest = !has_quest;
                    if(props->has_quest){
                        nk_layout_row_dynamic(ctx, 32, 2);
                        nk_label(ctx, "Quest ID", NK_TEXT_ALIGN_LEFT);
                        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_id, sizeof(TextLine), nk_filter_default);

                        nk_label(ctx, "Prompt Text", NK_TEXT_ALIGN_LEFT);
                        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_prompt, sizeof(TextLine), nk_filter_default);

                        nk_label(ctx, "In Progress Msg Text", NK_TEXT_ALIGN_LEFT);
                        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_progress, sizeof(TextLine), nk_filter_default);
                        nk_layout_row_dynamic(ctx, 32, 1);

                        int prev_type = NpcEditor.quest_type;
                        NpcEditor.quest_type = nk_combo(ctx, quest_types, 2, NpcEditor.quest_type, 20, nk_vec2(200,150));

                        if(prev_type != NpcEditor.quest_type){
                            NpcEditor.quest_tag = 0;
                        }

                        if(NpcEditor.quest_type == 0){
                            NpcEditor.quest_tag = nk_combo(ctx, enemy_types, ENEMY_TYPE_COUNT-1, NpcEditor.quest_tag, 20, nk_vec2(200,150));
                            nk_property_int(ctx, "Amount", 1, &NpcEditor.completion_progress, 1000, 1, 1);
                        } else {
                            nk_property_int(ctx, "Flag", 1, &NpcEditor.quest_tag, 255, 1, 1);
                        }
                                            
                        if(nk_button_label(ctx, "Save Quest")){
                            SJson* nquest = sj_object_new();
                            sj_object_insert(nquest, "id", sj_new_str(NpcEditor.quest_id));
                            sj_object_insert(nquest, "incomplete_text", sj_new_str(NpcEditor.quest_prompt));
                            sj_object_insert(nquest, "progress_msg", sj_new_str(NpcEditor.quest_progress));
                            sj_object_insert(nquest, "type", sj_new_int(NpcEditor.quest_type));
                            sj_object_insert(nquest, "tag", sj_new_int(NpcEditor.quest_tag));
                            sj_object_insert(nquest, "complete", sj_new_int(NpcEditor.completion_progress));

                            sj_object_delete_key(props->npc_json, "quest");
                            sj_object_insert(props->npc_json, "quest", nquest);
                        }
                    }

                    int is_shop = !props->is_shop;
                    nk_checkbox_label(ctx, "Is Shop", &is_shop);
                    props->is_shop = !is_shop;

                    if(props->is_shop){
                        props->sell_type = nk_combo(ctx, consumable_types, CONSUMABLE_COUNT, props->sell_type, 20, nk_vec2(200,150));
                        nk_property_int(ctx, "Price", 1, &props->sell_price, 255, 1, 1);
                    }

                    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, resource_location, sizeof(TextLine), nk_filter_default);
                    if(nk_button_label(ctx, "Save Npc")){
                        sj_object_delete_key(props->npc_json, "is_shop");
                        sj_object_insert(props->npc_json, "is_shop", sj_new_int(props->is_shop));
                        if(props->is_shop){
                            sj_object_delete_key(props->npc_json, "item");
                            SJson* item = sj_array_new();
                            sj_array_append(item, sj_new_int(props->sell_type));
                            sj_array_append(item, sj_new_int(props->sell_price));
                            sj_object_insert(props->npc_json, "item", item);
                        }

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

    if(*gamestate == 1){ // main menu
        nk_style_push_color(ctx, &ctx->style.window.background, nk_rgba(0,0,0,0));
        nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2(0,0));
        nk_begin(ctx, "sm_bg", nk_rect(0,0,1200,720), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_NO_INPUT);
        nk_layout_row_static(ctx, 720, 1200, 1);
        nk_image(ctx, nk_image_ptr(start_bg->texture));
        nk_end(ctx);
        nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,0)));
        nk_begin(ctx, "start_menu", nk_rect(425, 260, 350, 200), 0);
        nk_layout_row_dynamic(ctx, 32, 1);
        if(nk_button_label(ctx, "Start")){
            *gamestate = 0;
            audio_play_mod();
        }
        if(nk_button_label(ctx, "Quit")){
            *gamestate = 3;
        }
        nk_end(ctx);
        nk_style_pop_vec2(ctx);
        nk_style_pop_color(ctx);
        nk_style_pop_style_item(ctx);
    }
}

int game_paused(){
    return is_paused;
}

void menu_draw(){
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}