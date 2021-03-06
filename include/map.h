#ifndef __MAP_H__
#define __MAP_H__
#include "gf2d_sprite.h"
#include "gfc_list.h"
#include "rect.h"
#include "tracker.h"
#include "pg_entity.h"

typedef enum {
    POINT,
    RECT,
    CIRCLE
} ColType;

typedef struct  {
    TextLine load_map;
    Rect area;
    int dest_warp;
    int locked;
} Warp;

typedef struct {
    Uint8 is_town;
    Sprite* background;
    Sprite* decoration;
    Sprite* foreground;
 
    Uint32 col_count;
    Rect collision[80];
    Uint32 warp_count;
    Warp warps[15];
    
    TextLine name; //TODO: replace this with a hash?
    int map_width;
    int map_height;
    Vector2D player_spawn;
} Map;

//TODO: Write free func

void map_load(char* name);
void map_cleanup();
void map_new();
void map_save(char* path);

void map_manager_notify_editing(Uint8 is_editing);

void map_editor_notify_add(Rect to_add);
void map_editor_notify_delete();

void map_editor_notify_add_warp(Warp to_add);
void map_editor_notify_delete_warp();

void map_editor_notify_can_select(int select);

void map_editor_notify_set_bgm(char* path);
void map_editor_notify_set_bg(char* path);
void map_editor_notify_set_fg(char* path);
void map_editor_notify_set_deco(char* path);
void map_editor_notify_set_dimensions(Vector2D dimensions);

Uint8 collide_worldp(Vector2D p);
Uint8 ent_collide_world(Entity* ent);

void map_draw_fg(Map* m);
void map_draw_bg(Map* m);

void map_manager_draw_fg();
void map_manager_draw_bg();

int current_map_width();
int current_map_height();

void try_warp(Rect r);
void map_manager_unlock_warp(int warp);

void map_manager_update();
void map_manager_play_bgm();
Vector2D map_get_player_spawn();

#endif