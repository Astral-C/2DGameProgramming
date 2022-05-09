#include <string.h>
#include "simple_json_array.h"
#include "simple_logger.h"
#include "gfc_input.h"
#include "gf2d_graphics.h"
#include "menu.h"
#include "player.h"
#include "enemy.h"
#include "inventory.h"
#include "interactable.h"
#include "npc.h"
#include "audio.h"
#include "camera.h"
#include "map.h"
#include "quest.h"
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

typedef enum {
	REGULAR,
	CRAFTING,
	QUESTS
} PauseMenus;

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
static PauseMenus current_pause_menu = 0;
static struct nk_scroll quest_scroll;
static Vector2D cursor;

static const char* enemy_types[] = {
	"Bat",
	"Skull",
	"Golem",
	"Mushroom",
	"Magician",
	"Spawner"
};

static const char* interactable_types[] = {
	"Crate",
	"Scale Platforms",
	"Door Unlocker",
	"Timer Platforms",
	"Falling Platforms"
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
	TextLine quest_name;
	TextBlock quest_description;
	int quest_type;
	int quest_tag;
	int completion_progress;
	int quest_loaded;
} NpcEditor = {0};

ConfigData* EnemyConfigs;

static struct {
	TextLine bgm_path;
	TextLine bg_path;
	TextLine fg_path;
	TextLine deco_path;
	TextLine map_path;

	Warp* selected_warp;
	Rect* selected;

	Rect new_collision;
	Warp new_warp;
	int enemy_spawn_type;
	int interactable_spawn_type;
} MapEditor = {0};

static const char* craft_type_names[4] = {"Red Root", "Green Root", "Blue Root", "Yellow Root"};
static struct {
	CraftType item_1;
	CraftType item_2;
} CraftMenu = {0};

void map_editor_notify_selected(Rect* rect){
	MapEditor.selected = rect;
}

void map_editor_notify_selected_warp(Warp* warp){
	MapEditor.selected_warp = warp;
}

void map_editor_notify_load(char* bgm, char* bg, char* fg, char* deco, char* map_path){
	if(bgm != NULL){
		gfc_line_cpy(MapEditor.bgm_path, bgm);
	}
	if(bg != NULL){
		gfc_line_cpy(MapEditor.bg_path, bg);
	}
	if(fg != NULL){
		gfc_line_cpy(MapEditor.fg_path, fg);
	}
	if(deco != NULL){
		gfc_line_cpy(MapEditor.deco_path, deco);
	}
	gfc_line_cpy(MapEditor.map_path, map_path);
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

	cursor = vector2d(100,100);

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
		SDL_ShowCursor(SDL_ENABLE);
		is_paused = 1;
		audio_pause_mod();
	}

	if(is_paused){
		nk_style_push_color(ctx, &ctx->style.window.background, nk_rgba(0,0,0,100));
		nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2(0,0));
		nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, nk_style_item_color(nk_rgba(0,0,0,100)));
		nk_begin(ctx, "pm_bg", nk_rect(0, 0, 1200, 720), NK_WINDOW_NOT_INTERACTIVE);
		nk_end(ctx);
		nk_style_pop_vec2(ctx);
		
		nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2(50,25));
		
		switch (current_pause_menu)
		{
		case REGULAR:
			nk_begin(ctx, "pause_menu", nk_rect(425, 230, 350, 260), 0);
			nk_layout_row_static(ctx, 32, 240, 1);
			if(nk_button_label(ctx, "Continue")){
				is_paused = 0;
				map_editor_enabled = 0;
				*gamestate = 0;
				SDL_ShowCursor(SDL_DISABLE);
				map_manager_notify_editing(0);
				set_camera_target(entity_manager_get_player());
				audio_play_mod();
			}
			if(nk_button_label(ctx, "Craft")){
				current_pause_menu = CRAFTING;	
			}
			if(nk_button_label(ctx, "Quests")){
				current_pause_menu = QUESTS;	
			}
			if(nk_button_label(ctx, "Save")){
				SJson *save, *player, *inventory;
				save = sj_object_new();
				player = sj_object_new();
				inventory = sj_object_new();
				
				player_save(player);
				inventory_save(inventory);

				sj_object_insert(save, "player", player);
				sj_object_insert(save, "inventory", inventory);
				sj_object_insert(save, "map", sj_new_str(MapEditor.map_path));

				sj_save(save, "save.json");   
				sj_free(save);
			}
			nk_layout_row_static(ctx, 32, 240, 1);
			if(nk_button_label(ctx, "Edit Map")){
				*gamestate = 2;
				map_editor_enabled = 1;
				is_paused = 0;

				SDL_ShowCursor(SDL_ENABLE);
				map_manager_notify_editing(1);
				set_camera_target(NULL);
			}
			nk_layout_row_static(ctx, 32, 240, 1);
			if(nk_button_label(ctx, "Quit")){
				*gamestate = 3;
			}
			nk_end(ctx);

			break;
		
		case CRAFTING:
			nk_begin(ctx, "craft_menu", nk_rect(425, 260, 350, 200), 0);
			
			nk_layout_row_dynamic(ctx, 32, 2);

			Sprite* craftables_sprite = inventory_manager_craftables_img();
			Sprite* i1c_img = inv_get_craftable_count(CraftMenu.item_1);
			Sprite* i2c_img = inv_get_craftable_count(CraftMenu.item_2);

			struct nk_vec2 pos = nk_widget_position(ctx);
			gf2d_sprite_draw(craftables_sprite, vector2d(pos.x, pos.y - 116), NULL, NULL, NULL, NULL, NULL, CraftMenu.item_1);
			gf2d_sprite_draw(craftables_sprite, vector2d(pos.x + 128, pos.y - 116), NULL, NULL, NULL, NULL, NULL, CraftMenu.item_2);

			gf2d_sprite_draw_image(i1c_img, vector2d(pos.x + 32, pos.y));
			gf2d_sprite_draw_image(i2c_img, vector2d(pos.x + 160, pos.y));

			nk_layout_row_dynamic(ctx, 32, 2);
			CraftMenu.item_1 = nk_combo(ctx, craft_type_names, CRAFTABLE_COUNT, CraftMenu.item_1, 25, nk_vec2(250, 200));
			CraftMenu.item_2 = nk_combo(ctx, craft_type_names, CRAFTABLE_COUNT, CraftMenu.item_2, 25, nk_vec2(250, 200));
			
			nk_layout_row_static(ctx, 32, 240, 1);
			if(nk_button_label(ctx, "Craft")){
				inventory_try_craft(CraftMenu.item_1, CraftMenu.item_2);
			}

			if(nk_button_label(ctx, "Back")){
				current_pause_menu = REGULAR;	
			}
			nk_end(ctx);
			break;

		case QUESTS:
			nk_begin(ctx, "quest_menu", nk_rect(410, 260, 370, 200), 0);

			nk_layout_row_dynamic(ctx, 32, 1);

			if(nk_button_label(ctx, "Back")){
				current_pause_menu = REGULAR;	
			}
			
			nk_layout_row_dynamic(ctx, 125, 1);
			nk_group_scrolled_begin(ctx, &quest_scroll, "Quests", 0);
			QuestManager* quests = get_quest_manager();
			if(quests != NULL){
				char progress[10];
				for (int i = 0; i < gfc_list_get_count(quests->registered_quest_ids); i++){
					nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);
					Quest* q = (Quest*)gfc_hashmap_get(quests->available_quests, gfc_list_get_nth(quests->registered_quest_ids, i));
					//what? how
					if(q == NULL) continue;
					nk_layout_row_push(ctx, 0.85f);
					if(q == quests->active_quest){
						nk_label(ctx, q->name, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);
					} else {
						if(nk_button_label(ctx, q->name)){
							activate_quest(q->id);
						}
					
					}
					if(q->type == ET_KillEnemy){
						snprintf(progress, sizeof(progress), "%2d/%2d", q->progress, q->completion_progress);
					} else if(q->type == ET_EnableFlag) {
						snprintf(progress, sizeof(progress), "%s", (q->is_complete ? "[x]" : "[ ]"));
					}
					nk_layout_row_push(ctx, 0.15f);
					nk_label(ctx, progress, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);
					nk_layout_row_end(ctx);
				}
			}
			nk_group_scrolled_end(ctx);
			
			nk_end(ctx);

			nk_begin(ctx, "quest_menu_descr", nk_rect(365, 500, 475, 125), 0);
			nk_layout_row_dynamic(ctx, 64, 1);
			if(quests->active_quest != NULL) nk_label_wrap(ctx, quests->active_quest->description);
			nk_end(ctx);
			break;

		default:
			nk_begin(ctx, "default_menu", nk_rect(425, 260, 350, 200), 0);
			nk_layout_row_static(ctx, 32, 240, 1);
			if(nk_button_label(ctx, "Back")){
				current_pause_menu = REGULAR;	
			}
			nk_end(ctx);
			break;
		}
		
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
		if (nk_begin(ctx, "Debug", nk_rect(50, 50, 230, 250), NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE|NK_WINDOW_CLOSABLE)){
			nk_layout_row_dynamic(ctx, 32, 2);

			if(nk_button_label(ctx, "Pause/Play")){
				is_paused  = !is_paused;
			}

			nk_checkbox_label(ctx, "Show Hitboxes", &show_hitbox);
			entity_manager_set_draw_debug(!show_hitbox);

		}
		nk_end(ctx);
	}

	if(map_editor_enabled && *gamestate == 2){
		if (nk_begin(ctx, "Map Editor", nk_rect(50, 50, 360, 380), NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)){

			if(nk_tree_push(ctx, NK_TREE_TAB, "Collision", NK_MINIMIZED)){
				nk_layout_row_dynamic(ctx, 32, 2);
				if(nk_button_label(ctx, "Add")){
					if(MapEditor.selected == NULL){
						map_editor_notify_add(MapEditor.new_collision);
					} else{
						map_editor_notify_add(*MapEditor.selected);
					}
				}

				if(nk_button_label(ctx, "Delete")){
					map_editor_notify_delete(MapEditor.selected);
				}

				if(MapEditor.selected != NULL){
					nk_property_float(ctx, "x", 0, &MapEditor.selected->pos.x, current_map_width(), 16, 16);
					nk_property_float(ctx, "y", 0, &MapEditor.selected->pos.y, current_map_height(), 16, 16);
					nk_property_float(ctx, "w", 0, &MapEditor.selected->size.x, current_map_width(), 16, 16);
					nk_property_float(ctx, "h", 0, &MapEditor.selected->size.y, current_map_height(), 16, 16);
				} else {
					nk_property_float(ctx, "x", 0, &MapEditor.new_collision.pos.x, current_map_width(), 16, 16);
					nk_property_float(ctx, "y", 0, &MapEditor.new_collision.pos.y, current_map_height(), 16, 16);
					nk_property_float(ctx, "w", 0, &MapEditor.new_collision.size.x, current_map_width(), 16, 16);
					nk_property_float(ctx, "h", 0, &MapEditor.new_collision.size.y, current_map_height(), 16, 16);
				}
				nk_tree_pop(ctx);
			}
			
			if(nk_tree_push(ctx, NK_TREE_TAB, "Warp", NK_MINIMIZED)){
				nk_layout_row_dynamic(ctx, 32, 2);

				if(nk_button_label(ctx, "Add")){
					map_editor_notify_add_warp(MapEditor.new_warp);
				}

				if(nk_button_label(ctx, "Delete")){
					map_editor_notify_delete_warp(MapEditor.selected);
				}

				if(MapEditor.selected_warp != NULL){
					nk_property_float(ctx, "wx", 0, &MapEditor.selected_warp->area.pos.x, current_map_width(), 16, 16);
					nk_property_float(ctx, "wy", 0, &MapEditor.selected_warp->area.pos.y, current_map_height(), 16, 16);
					nk_property_float(ctx, "ww", 0, &MapEditor.selected_warp->area.size.x, current_map_width(), 16, 16);
					nk_property_float(ctx, "wh", 0, &MapEditor.selected_warp->area.size.y, current_map_height(), 16, 16);
					nk_property_int(ctx, "Dest Warp", 0, &MapEditor.selected_warp->dest_warp, 100, 1, 1);
					nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.selected_warp->load_map, sizeof(TextLine), nk_filter_default);                    
					nk_layout_row_dynamic(ctx, 32, 1);
					nk_checkbox_label(ctx, "Locked", &MapEditor.selected_warp->locked);
				} else {
					nk_property_float(ctx, "wx", 0, &MapEditor.new_warp.area.pos.x, current_map_width(), 16, 16);
					nk_property_float(ctx, "wy", 0, &MapEditor.new_warp.area.pos.y, current_map_height(), 16, 16);
					nk_property_float(ctx, "ww", 0, &MapEditor.new_warp.area.size.x, current_map_width(), 16, 16);
					nk_property_float(ctx, "wh", 0, &MapEditor.new_warp.area.size.y, current_map_height(), 16, 16);
					nk_property_int(ctx, "Dest Warp", 0, &MapEditor.new_warp.dest_warp, 100, 1, 1);
					nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.new_warp.load_map, sizeof(TextLine), nk_filter_default);
					nk_layout_row_dynamic(ctx, 32, 1);
					nk_checkbox_label(ctx, "Locked", &MapEditor.new_warp.locked);
				}
				nk_tree_pop(ctx);
			}

			if(nk_tree_push(ctx, NK_TREE_TAB, "Enemy", NK_MINIMIZED)){
				nk_label(ctx, "Enemy Type", NK_TEXT_ALIGN_LEFT);
				MapEditor.enemy_spawn_type = nk_combo(ctx, enemy_types, ENEMY_TYPE_COUNT, MapEditor.enemy_spawn_type, 25, nk_vec2(300,150));
				if(nk_button_label(ctx, "Add")){
					Entity* new_ent = NULL;
					switch (MapEditor.enemy_spawn_type){
					case 0:
						new_ent = bat_new();
						break;
					case 1:
						new_ent = skull_new();
						break;
					case 2:
						new_ent = golem_new();
						break;
					case 3:
						new_ent = mushroom_new();
						break;
					case 4:
						new_ent = magician_new();
						break;
					
					default:
						break;
					}
					
					Vector2D spawn_pos;
					spawn_pos = get_camera_pos();
					spawn_pos.x += 1200/2;
					spawn_pos.y += 720/2;
					vector2d_copy(new_ent->position, spawn_pos);
					vector2d_copy(new_ent->hurtbox.pos, new_ent->position);

				}
				nk_tree_pop(ctx);
			}

			if(nk_tree_push(ctx, NK_TREE_TAB, "Interactable", NK_MINIMIZED)){
				nk_label(ctx, "Interactale Type", NK_TEXT_ALIGN_LEFT);
				MapEditor.interactable_spawn_type = nk_combo(ctx, interactable_types, INTERACTABLE_TYPE_COUNT, MapEditor.interactable_spawn_type, 25, nk_vec2(300,150));
				if(nk_button_label(ctx, "Add")){
					
					Vector2D spawn_pos;
					spawn_pos = get_camera_pos();
					spawn_pos.x += 1200/2;
					spawn_pos.y += 720/2;

					switch (MapEditor.interactable_spawn_type){
					case CRATE:
						spawn_crate(spawn_pos, 5, 0);
						break;
					case SCALE_PLATFORM:{
						Vector2D spawn_pos_2;
						vector2d_add(spawn_pos_2, spawn_pos, vector2d(64, 0));
						spawn_weight_platforms(spawn_pos, spawn_pos_2, 100);
						break;
					}
					case DOOR_BUTTON:
						spawn_unlock_door_button(spawn_pos, 0);
						break;
					case TIMER_PLATFORM_BUTTON:
						spawn_timed_platforms_new(spawn_pos, 5);
						break;
					case FALLING_PLATFORM:
						spawn_falling_platform(spawn_pos, 200);
						break;
					
					default:
						break;
					}

				}
				nk_tree_pop(ctx);
			}

			nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);            
			nk_layout_row_push(ctx, 0.2);
			nk_label(ctx, "Map BGM", NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);
			nk_layout_row_push(ctx, 0.55);
			nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.bgm_path, sizeof(TextLine), nk_filter_default);
			nk_layout_row_push(ctx, 0.2);
			if(nk_button_label(ctx, "Set")){
				map_editor_notify_set_bgm(MapEditor.bgm_path);
			}
			nk_layout_row_end(ctx);

			nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);            
			nk_layout_row_push(ctx, 0.2);
			nk_label(ctx, "BG Path", NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);
			nk_layout_row_push(ctx, 0.55);
			nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.bg_path, sizeof(TextLine), nk_filter_default);
			nk_layout_row_push(ctx, 0.2);
			if(nk_button_label(ctx, "Set")){
				map_editor_notify_set_bg(MapEditor.bg_path);
			}
			nk_layout_row_end(ctx);
			
			nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);
			nk_layout_row_push(ctx, 0.2);
			nk_label(ctx, "FG Path", NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);
			nk_layout_row_push(ctx, 0.55);
			nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.fg_path, sizeof(TextLine), nk_filter_default);
			nk_layout_row_push(ctx, 0.2);
			if(nk_button_label(ctx, "Set")){
				map_editor_notify_set_fg(MapEditor.fg_path);
			}
			nk_layout_row_end(ctx);
			
			nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);
			nk_layout_row_push(ctx, 0.2);
			nk_label(ctx, "Deco Path", NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);
			nk_layout_row_push(ctx, 0.55);
			nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.deco_path, sizeof(TextLine), nk_filter_default);
			nk_layout_row_push(ctx, 0.2);
			if(nk_button_label(ctx, "Set")){
				map_editor_notify_set_deco(MapEditor.deco_path);
			}
			nk_layout_row_end(ctx);

			nk_layout_row_begin(ctx, NK_DYNAMIC, 32, 2);
			nk_layout_row_push(ctx, 0.2);
			nk_label(ctx, "Map Path", NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);
			nk_layout_row_push(ctx, 0.8);
			nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, MapEditor.map_path, sizeof(TextLine), nk_filter_default);
			nk_layout_row_end(ctx);

			nk_layout_row_dynamic(ctx, 32, 2);
			if(nk_button_label(ctx, "New Map")){
				map_editor_notify_selected(NULL);
				map_editor_notify_selected_warp(NULL);
				entity_manager_clear(entity_manager_get_player());
				map_cleanup();
			
				map_new();
			}

			if(nk_button_label(ctx, "Save Map")){
				map_save(MapEditor.map_path);
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

					case ENEMY_BAT: {
						int move_timer;

						nk_label(ctx, "Bat Config", NK_TEXT_ALIGN_LEFT);
						health = EnemyConfigs->BatConfig.health;
						move_timer = EnemyConfigs->BatConfig.move_timer;

						nk_property_int(ctx, "Health", 1, &health, 255, 1, 1);
						nk_property_int(ctx, "Move Timer", 1, &move_timer, 255, 1, 1);

						EnemyConfigs->BatConfig.health = (Uint8)health;
						EnemyConfigs->BatConfig.move_timer = (Uint16)move_timer;

						if(nk_button_label(ctx, "Save Config")){
							SJson* new_bat_conf = sj_object_new();
							sj_object_insert(new_bat_conf, "health", sj_new_int(health));
							sj_object_insert(new_bat_conf, "move_timer", sj_new_int(move_timer));
							sj_save(new_bat_conf, "bat_def.json");
						}
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
					if(nk_tree_push(ctx, NK_TREE_TAB, "Text Pages", NK_MINIMIZED)){						
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
							// ???  this is defined???
							sj_array_replace_nth(sj_object_get_value(props->npc_json, "pages"), NpcEditor.selected_page, str);
						}
						nk_tree_pop(ctx);
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
							gfc_line_cpy(NpcEditor.quest_name, sj_get_string_value(sj_object_get_value(quest, "name")));
							gfc_line_cpy(NpcEditor.quest_description, sj_get_string_value(sj_object_get_value(quest, "description")));
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
					if(props->has_quest && nk_tree_push(ctx, NK_TREE_TAB, "Quest", NK_MINIMIZED)){
						nk_layout_row_dynamic(ctx, 32, 2);
						nk_label(ctx, "Quest ID", NK_TEXT_ALIGN_LEFT);
						nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_id, sizeof(TextLine), nk_filter_default);

						nk_label(ctx, "Name", NK_TEXT_ALIGN_LEFT);
						nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_name, sizeof(TextLine), nk_filter_default);

						nk_label(ctx, "Description", NK_TEXT_ALIGN_LEFT);
						nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, NpcEditor.quest_description, sizeof(TextLine), nk_filter_default);

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
							sj_object_insert(nquest, "name", sj_new_str(NpcEditor.quest_name));
							sj_object_insert(nquest, "description", sj_new_str(NpcEditor.quest_description));
							sj_object_insert(nquest, "incomplete_text", sj_new_str(NpcEditor.quest_prompt));
							sj_object_insert(nquest, "progress_msg", sj_new_str(NpcEditor.quest_progress));
							sj_object_insert(nquest, "type", sj_new_int(NpcEditor.quest_type));
							sj_object_insert(nquest, "tag", sj_new_int(NpcEditor.quest_tag));
							sj_object_insert(nquest, "complete", sj_new_int(NpcEditor.completion_progress));

							sj_object_delete_key(props->npc_json, "quest");
							sj_object_insert(props->npc_json, "quest", nquest);
						}
						nk_tree_pop(ctx);
					}

					int is_shop = !props->is_shop;
					nk_checkbox_label(ctx, "Is Shop", &is_shop);
					props->is_shop = !is_shop;

					if(props->is_shop){
						props->sell_type = nk_combo(ctx, consumable_types, CONSUMABLE_COUNT, props->sell_type, 20, nk_vec2(200,150));
						nk_property_int(ctx, "Price", 1, (int*)&props->sell_price, 255, 1, 1);
					}

					nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, resource_location, sizeof(TextLine), nk_filter_default);
					if(nk_button_label(ctx, "Save Npc")){
						sj_object_delete_key(props->npc_json, "is_shop");
						sj_object_insert(props->npc_json, "is_shop", sj_new_int(props->is_shop));

						SJson* interact_box = sj_array_new();
						sj_array_append(interact_box, sj_new_float(current->hurtbox.pos.x / 4));
						sj_array_append(interact_box, sj_new_float(current->hurtbox.pos.y / 4));
						sj_array_append(interact_box, sj_new_float(current->hurtbox.size.x / 4));
						sj_array_append(interact_box, sj_new_float(current->hurtbox.size.y / 4));
						sj_object_delete_key(props->npc_json, "interact");
						sj_object_insert(props->npc_json, "interact", interact_box);


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

				case ENT_INTERACTABLE: {
					InteractableType e = *((InteractableType*)current->data);
					switch (e){
					case CRATE: {
						CrateData* d = (CrateData*)current->data;
						nk_layout_row_dynamic(ctx, 32, 1);
						nk_property_int(ctx, "Health", 1, &current->health, 255, 1, 1);
						d->drop = nk_combo(ctx, craft_type_names, CRAFTABLE_COUNT, d->drop, 25, nk_vec2(300,150));
						nk_property_float(ctx, "Weight", 1, &d->weight, 100, 1, 1);
						break;
					}

					case FALLING_PLATFORM: {
						FallingPlatformData* d = (FallingPlatformData*)current->data;
						nk_layout_row_dynamic(ctx, 32, 1);
						nk_property_float(ctx, "Time Before Fall", 1, &d->time_before_fall, 600, 1, 1);
						break;
					}
					
					case SCALE_PLATFORM: {
						WeightedPlatformData* d = (WeightedPlatformData*)current->data;
						nk_layout_row_dynamic(ctx, 32, 1);
						nk_property_float(ctx, "Max Drop", 1, &d->max_drop, 200, 1, 1);

						WeightedPlatformData* d2 = (WeightedPlatformData*)current->owner->data;
						d2->max_drop = d->max_drop;

						break;
					}

					case DOOR_BUTTON:{
						WarpUnlockData* d = (WarpUnlockData*)current->data;
						nk_layout_row_dynamic(ctx, 32, 1);
						nk_property_int(ctx, "Unlock Warp", 1, &d->warp, map_manager_get_warp_count(), 1, 1);
						break;
					}

					case TIMER_PLATFORM_BUTTON:{
						TimePlatformButtonData* d = (TimePlatformButtonData*)current->data;
						nk_layout_row_dynamic(ctx, 32, 1);
						nk_property_int(ctx, "Appear Time Max", 1, &d->time_max, 1000, 1, 1);
						break;
					}
					
					case TIMER_PLATFORM : {
						TimedPlatformData* d = (TimedPlatformData*)current->data;
						nk_layout_row_dynamic(ctx, 32, 1);
						nk_property_int(ctx, "Floats", 1, &d->move, 600, 1, 1);
						nk_property_int(ctx, "Float Amplitude", 1, &d->amplitude, 600, 1, 1);
						nk_property_float(ctx, "Float Seq Offset", 1, &d->offset, 600, 1, 1);
						break;
					}

					default:
						break;
					}
				}

				default:
					break;
				}

				nk_layout_row_dynamic(ctx, 32, 1);
				if(nk_button_label(ctx, "Delete") && current->type != ENT_PLAYER){
					entity_free(current);
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
			SDL_ShowCursor(SDL_DISABLE);
			audio_play_mod();
		}
		//This should show a list of saves
		if(nk_button_label(ctx, "Load")){
			SJson* save = sj_load("save.json");
			if(save != NULL){
				Vector2D position;
				SJson *player, *pos_arr, *inventory, *pconsumables, *pcraftables;  
				float stamina;
				int i, health, money, consumables[CONSUMABLE_COUNT], craftables[CRAFTABLE_COUNT];

				map_cleanup();
				entity_manager_clear(entity_manager_get_player());
				char* save_file_map = (char*)sj_get_string_value(sj_object_get_value(save, "map"));
				map_load(save_file_map);

				player = sj_object_get_value(save, "player");
				if(player){
					sj_get_integer_value(sj_object_get_value(player, "health"), &health);
					sj_get_float_value(sj_object_get_value(player, "stamina"), &stamina);
					sj_get_integer_value(sj_object_get_value(player, "money"), &money);
					
					pos_arr = sj_object_get_value(player, "position");
					sj_get_float_value(sj_array_get_nth(pos_arr, 0), &position.x);
					sj_get_float_value(sj_array_get_nth(pos_arr, 1), &position.y);
					player_load(entity_manager_get_player(), position, health, stamina, money);
				}

				inventory = sj_object_get_value(save, "inventory");
				if(inventory){
					pconsumables = sj_object_get_value(inventory, "consumables");
					pcraftables = sj_object_get_value(inventory, "craftables");
					for (i = 0; i < sj_array_get_count(pconsumables); i++){
						sj_get_integer_value(sj_array_get_nth(pconsumables, i), &consumables[i]);
					}
					for (i = 0; i < sj_array_get_count(pcraftables); i++){
						sj_get_integer_value(sj_array_get_nth(pcraftables, i), &craftables[i]);
					}
					inventory_load(consumables, craftables);
				}


				*gamestate = 0;
				SDL_ShowCursor(SDL_DISABLE);
				sj_free(save);
			}
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