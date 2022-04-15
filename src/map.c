#include "map.h"
#include "camera.h"
#include "simple_json.h"
#include "simple_logger.h"
#include "enemy.h"
#include "npc.h"

typedef struct {
    Map current_map;
    int should_warp;
    Warp warp_target;
    TextLine map_jsn_path;
} MapManager;

static MapManager map_manager = {0};

int current_map_width(){
    return map_manager.current_map.map_width;
}

int current_map_height(){
    return map_manager.current_map.map_height;
}

void map_load(char* map_def){

    SJson* jsn;


    jsn = sj_load(map_def);
    
    if(!jsn){
        slog("Unable to open map def %s");
        return;
    }
    
    gfc_line_cpy(map_manager.map_jsn_path, map_def);

    /* Note: I may change this later to build the map from a tiled json export, we'll see */

    char* map_name = (char*)sj_get_string_value(sj_object_get_value(jsn, "name"));
    char* map_fg = (char*)sj_get_string_value(sj_object_get_value(jsn, "map_fg"));
    char* map_deco = (char*)sj_get_string_value(sj_object_get_value(jsn, "map_deco"));
    char* map_bg = (char*)sj_get_string_value(sj_object_get_value(jsn, "map_bg"));
    short is_town = 0;
    sj_get_bool_value(sj_object_get_value(jsn, "is_town"), &is_town);
    SJson* collision = sj_object_get_value(jsn, "collision");
    SJson* spawners = sj_object_get_value(jsn, "spawners");
    SJson* enemies = sj_object_get_value(jsn, "enemies");
    SJson* npcs = sj_object_get_value(jsn, "npcs");
    SJson* warps = sj_object_get_value(jsn, "warps");

    SJson* player_spawn = sj_object_get_value(jsn, "player_spawn");
    
    if(player_spawn){
        int x, y;
        sj_get_integer_value(sj_array_get_nth(player_spawn, 0), &x);
        sj_get_integer_value(sj_array_get_nth(player_spawn, 1), &y);
        entity_manager_get_player()->position = vector2d(x << 2, y << 2);
        map_manager.current_map.player_spawn = vector2d(x << 2, y << 2);
    }

    if(!map_name){
        slog("Couldn't load map name from def %s", map_def);
        sj_free(jsn);
        return;
    }

    int collision_rect_count = sj_array_get_count(collision);
    map_manager.current_map.rect_count = collision_rect_count;

    if(map_manager.current_map.collision != NULL) free(map_manager.current_map.collision);
    map_manager.current_map.collision = gfc_allocate_array(sizeof(Rect), map_manager.current_map.rect_count);
    
    for (int cr = 0; cr < collision_rect_count; cr++){
        SJson* rect = sj_array_get_nth(collision, cr);
        int x, y, w, h;
        sj_get_integer_value(sj_array_get_nth(rect, 0), &x);
        sj_get_integer_value(sj_array_get_nth(rect, 1), &y);
        sj_get_integer_value(sj_array_get_nth(rect, 2), &w);
        sj_get_integer_value(sj_array_get_nth(rect, 3), &h);
        map_manager.current_map.collision[cr].pos.x = (float)(x<<2);
        map_manager.current_map.collision[cr].pos.y = (float)(y<<2);
        map_manager.current_map.collision[cr].size.x = (float)(w<<2);
        map_manager.current_map.collision[cr].size.y = (float)(h<<2);
    }

    /*
    
        perhaps maps should have a list of spawners/warps so it can kill it's own entities on cleanup instead of needing to kill all entities?
    
    */

    if(!is_town){
        int spawner_count = sj_array_get_count(spawners);
        
        for (int cr = 0; cr < spawner_count; cr++){
            SJson* spawner = sj_array_get_nth(spawners, cr);
            int type, count, interval, max_entities, x, y;

            sj_get_integer_value(sj_object_get_value(spawner, "spawn_type"), &type);
            sj_get_integer_value(sj_object_get_value(spawner, "spawn_amount"), &count);
            sj_get_integer_value(sj_object_get_value(spawner, "spawn_interval"), &interval);
            sj_get_integer_value(sj_object_get_value(spawner, "max_entities"), &max_entities);

            SJson* position = sj_object_get_value(spawner, "position");
            sj_get_integer_value(sj_array_get_nth(position, 0), &x);
            sj_get_integer_value(sj_array_get_nth(position, 1), &y);

            Entity* ent = enemy_spawner_new(type, count, interval, max_entities);
            ent->position.x = (float)(x << 2);
            ent->position.y = (float)(y << 2);

        }

        int enemy_count = sj_array_get_count(enemies);
        
        for (int cr = 0; cr < enemy_count; cr++){
            SJson* enemy = sj_array_get_nth(enemies, cr);
            int type, x, y;

            sj_get_integer_value(sj_object_get_value(enemy, "type"), &type);

            SJson* position = sj_object_get_value(enemy, "position");
            sj_get_integer_value(sj_array_get_nth(position, 0), &x);
            sj_get_integer_value(sj_array_get_nth(position, 1), &y);

            Entity* ent = NULL;
            switch (type)
            {
            case ENEMY_GOLEM:
                ent = golem_new();
                break;

            case ENEMY_MUSHROOM:
                ent = mushroom_new();
                break;

            case ENEMY_MAGICIAN:
                ent = magician_new();
                break;
            }

            if(ent){
                ent->position.x = (float)(x << 2);
                ent->position.y = (float)(y << 2);
            }
        }
    }

    int warp_count = sj_array_get_count(warps);
    map_manager.current_map.warp_count = warp_count;

    if(map_manager.current_map.warps != NULL) free(map_manager.current_map.warps);
    map_manager.current_map.warps = gfc_allocate_array(sizeof(Warp), warp_count);    
    for (int cr = 0; cr < warp_count; cr++){
        SJson* warp = sj_array_get_nth(warps, cr);
        
        char* warp_path = (char*)sj_get_string_value(sj_object_get_value(warp, "map"));
        strncpy(map_manager.current_map.warps[cr].load_map, warp_path, sizeof(TextLine));

        SJson* dest = sj_object_get_value(warp, "dest_warp");
        if(!dest){
            map_manager.current_map.warps[cr].dest_warp = -1;  
        } else {
            sj_get_integer_value(dest, &map_manager.current_map.warps[cr].dest_warp);
        }

        SJson* rect = sj_object_get_value(warp, "box");
        int x, y, w, h;
        sj_get_integer_value(sj_array_get_nth(rect, 0), &x);
        sj_get_integer_value(sj_array_get_nth(rect, 1), &y);
        sj_get_integer_value(sj_array_get_nth(rect, 2), &w);
        sj_get_integer_value(sj_array_get_nth(rect, 3), &h);

        map_manager.current_map.warps[cr].area.pos.x = x << 2;
        map_manager.current_map.warps[cr].area.pos.y = y << 2;
        map_manager.current_map.warps[cr].area.size.x = w << 2;
        map_manager.current_map.warps[cr].area.size.y = h << 2;

    }


    int npc_count = sj_array_get_count(npcs);
    
    for (int cr = 0; cr < npc_count; cr++){
        SJson* npc = sj_array_get_nth(npcs, cr);
        char* path = (char*)sj_get_string_value(npc);
        //return Entity*
        npc_new(path);
    }


    if(map_manager.current_map.foreground){
        gf2d_sprite_free(map_manager.current_map.foreground);
        map_manager.current_map.foreground = NULL;
    }

    if(map_fg){
        map_manager.current_map.foreground = gf2d_sprite_load_image(map_fg);
    }

    if(map_manager.current_map.decoration){
        gf2d_sprite_free(map_manager.current_map.decoration);
        map_manager.current_map.decoration = NULL;
    }
    
    if(map_deco){
        map_manager.current_map.decoration = gf2d_sprite_load_image(map_deco);
    }

    if(map_manager.current_map.background){
        gf2d_sprite_free(map_manager.current_map.background);
        map_manager.current_map.background = NULL;
    }
    
    
    if(map_bg){
        map_manager.current_map.background = gf2d_sprite_load_image(map_bg);
        if(!map_manager.current_map.background){
            slog("couldnt load map background");
        }
    }

    map_manager.current_map.map_width = map_manager.current_map.background->frame_w * 4;
    map_manager.current_map.map_height = map_manager.current_map.background->frame_h * 4;

    sj_free(jsn);

}

void map_manager_update(){
    if(map_manager.should_warp == 1){
        entity_manager_clear(entity_manager_get_player());
        ui_manager_clear_ephemeral();
        map_load(map_manager.warp_target.load_map);
        if(map_manager.warp_target.dest_warp != -1){
            entity_manager_get_player()->position = map_manager.current_map.warps[map_manager.warp_target.dest_warp].area.pos;
            map_manager.current_map.player_spawn = map_manager.current_map.warps[map_manager.warp_target.dest_warp].area.pos;
        }
        map_manager.should_warp = 0;
    }
}

void try_warp(Rect r){
    for (size_t i = 0; i < map_manager.current_map.warp_count; i++){
        if(rect_collider(map_manager.current_map.warps[i].area, r)){
            map_manager.should_warp = 1;
            map_manager.warp_target = map_manager.current_map.warps[i];
            break;
        }
    }
}

void map_draw_fg(Map* m){
    if(!m) return;
    Vector2D cam = get_camera_pos();
    cam.x = -cam.x;
    cam.y = -cam.y;
    if(m->decoration) gf2d_sprite_draw(m->decoration, cam, get_camera_zoom(), NULL, NULL, NULL, NULL, 0);
    if(m->foreground) gf2d_sprite_draw(m->foreground, cam, get_camera_zoom(), NULL, NULL, NULL, NULL, 0);
}

void map_draw_bg(Map* m){
    if(!m || !m->background) return;
    Vector2D cam = get_camera_pos();
    cam.x = -cam.x;
    cam.y = -cam.y;
    gf2d_sprite_draw(m->background, cam, get_camera_zoom(), NULL, NULL, NULL, NULL, 0);
}

void map_manager_draw_fg(){
    map_draw_fg(&map_manager.current_map);
}

void map_manager_draw_bg(){
    map_draw_bg(&map_manager.current_map);
}

Uint8 collide_worldp(Vector2D p){
    for (int i = 0; i < map_manager.current_map.rect_count; i++){
        if(rect_collidep(p, map_manager.current_map.collision[i])){
            return 1;
        }
    }
    return 0;   
}

Uint8 ent_collide_world(Entity* ent){
    Rect r1 = ent->hurtbox;
    vector2d_add(r1.pos, ent->hurtbox.pos, ent->velocity);
    Uint8 col_data = 0;
    
    if(r1.pos.x < 0 || r1.pos.x + r1.size.x > map_manager.current_map.map_width){
        ent->velocity.x = 0;
        return 1;
    }
    
    if(r1.pos.y < 0 || r1.pos.y > map_manager.current_map.map_height){
        ent->velocity.y = 0;
        return COL_FLOOR;
    }

    for (int i = 0; i < map_manager.current_map.rect_count; i++){
        Rect r2 = map_manager.current_map.collision[i];
        Uint8 col_xl = r1.pos.x <= r2.pos.x + r2.size.x;
        Uint8 col_xr = r1.pos.x + r1.size.x >= r2.pos.x;
        Uint8 col_yu = r1.pos.y <= r2.pos.y + r2.size.y;
        Uint8 col_yd = r1.pos.y + r1.size.y >= r2.pos.y;
        if(col_xl && col_xr && col_yd && col_yu){
            if(col_yd && ent->velocity.y >= 0){
                ent->velocity.y = 0;
                ent->position.y = r2.pos.y - r1.size.y;
                col_data |= COL_FLOOR;
            }
            if(col_yu && ent->velocity.y < 0) ent->velocity.y = 0;
        }
    }   
    return col_data;
}

Vector2D map_get_player_spawn(){
    return map_manager.current_map.player_spawn;
}