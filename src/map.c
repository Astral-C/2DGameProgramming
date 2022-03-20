#include "map.h"
#include "camera.h"
#include "simple_json.h"
#include "simple_logger.h"
#include "enemy.h"

typedef struct {
    Map* maps;
    Uint8 current_map;
    Uint8 map_count;
} MapManager;

static MapManager map_manager = {0};

void map_manager_init(Uint8 map_count){
    if(map_count == 0){
        slog("No Maps to be allocated");
        return;
    }

    map_manager.maps = (Map*)gfc_allocate_array(sizeof(Map), map_count);

    if(map_manager.maps == NULL){
        slog("Unable to allocate maps");
        return;
    }

    map_manager.map_count = map_count;
    map_manager.current_map = 0; //TODO: Make this an option?
}

Map* map_new(char* name){
    for (size_t i = 0; i < map_manager.map_count; i++){
        if(map_manager.maps[i]._ref_count == 0){
            map_manager.maps[i]._ref_count = 1;
            gfc_line_cpy(map_manager.maps[i].name, name);
            return &map_manager.maps[i];
        }
    }
    slog("no maps available");

    return NULL;
}

Map* get_map_by_name(char* name){
    for (size_t i = 0; i < map_manager.map_count; i++){
        if(gfc_line_cmp(name, map_manager.maps[i].name) == 0){
            map_manager.maps[i]._ref_count++;
            return &map_manager.maps[i];
        }
    }
    return NULL;
}

/*
* Loads a map from a properly formatted map def json
* Returns Map* on success/found and NULL on failure
*/
Map* map_load(char* map_def){
    Map* map;
    SJson* jsn;

    jsn = sj_load(map_def);
    
    if(!jsn){
        slog("Unable to open map def %s");
        return NULL;
    }

    /* Note: I may change this later to build the map from a tiled json export, we'll see */

    const char* map_name = sj_get_string_value(sj_object_get_value(jsn, "name"));
    const char* map_fg = sj_get_string_value(sj_object_get_value(jsn, "map_fg"));
    const char* map_deco = sj_get_string_value(sj_object_get_value(jsn, "map_deco"));
    const char* map_bg = sj_get_string_value(sj_object_get_value(jsn, "map_bg"));
    SJson* collision = sj_object_get_value(jsn, "collision");
    SJson* spawners = sj_object_get_value(jsn, "spawners");
    SJson* enemies = sj_object_get_value(jsn, "enemies");

    if(!map_name){
        slog("Couldn't load map name from def %s", map_def);
        sj_free(jsn);
        return NULL;
    }

    map = get_map_by_name(map_name);

    if(map){
        sj_free(jsn);
        return map;
    }

    map = map_new(map_name);

    if(!map){
        sj_free(jsn);
        slog("couldn't make map");
        return NULL;
    }

    int collision_rect_count = sj_array_get_count(collision);
    map->rect_count = collision_rect_count;

    map->collision = gfc_allocate_array(sizeof(Rect), map->rect_count);
    
    for (int cr = 0; cr < collision_rect_count; cr++){
        SJson* rect = sj_array_get_nth(collision, cr);
        int x, y, w, h;
        sj_get_integer_value(sj_array_get_nth(rect, 0), &x);
        sj_get_integer_value(sj_array_get_nth(rect, 1), &y);
        sj_get_integer_value(sj_array_get_nth(rect, 2), &w);
        sj_get_integer_value(sj_array_get_nth(rect, 3), &h);
        map->collision[cr].pos.x = (float)(x<<2);
        map->collision[cr].pos.y = (float)(y<<2);
        map->collision[cr].size.x = (float)(w<<2);
        map->collision[cr].size.y = (float)(h<<2);
    }

    /*
    
        perhaps maps should have a list of spawners/warps so it can kill it's own entities on cleanup instead of needing to kill all entities?
    
    */

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


    if(map_fg){
        map->foreground = gf2d_sprite_load_image(map_fg);
    }

    if(map_deco){
        map->decoration = gf2d_sprite_load_image(map_deco);
    }

    if(map_bg){
        map->background = gf2d_sprite_load_image(map_bg);
        if(!map->background){
            slog("couldnt load map background");
        }
    }

    sj_free(jsn);

    return map;
}

void map_draw_fg(Map* m){
    if(!m || !m->decoration || !m->foreground) return;
    Vector2D cam = get_camera_pos();
    cam.x = -cam.x;
    cam.y = -cam.y;
    gf2d_sprite_draw(m->decoration, cam, get_camera_zoom(), NULL, NULL, NULL, NULL, 0);
    gf2d_sprite_draw(m->foreground, cam, get_camera_zoom(), NULL, NULL, NULL, NULL, 0);
}

void map_draw_bg(Map* m){
    if(!m || !m->background) return;
    Vector2D cam = get_camera_pos();
    cam.x = -cam.x;
    cam.y = -cam.y;
    gf2d_sprite_draw(m->background, cam, get_camera_zoom(), NULL, NULL, NULL, NULL, 0);
}

void map_manager_draw_fg(){
    map_draw_fg(&map_manager.maps[map_manager.current_map]);
}

void map_manager_draw_bg(){
    map_draw_bg(&map_manager.maps[map_manager.current_map]);
}

Uint8 collide_worldp(Vector2D p){
    for (int i = 0; i < map_manager.maps[map_manager.current_map].rect_count; i++){
        if(rect_collidep(p, map_manager.maps[map_manager.current_map].collision[i])){
            return 1;
        }
    }
    return 0;   
}

Uint8 ent_collide_world(Entity* ent){
    Rect r1 = ent->hurtbox;
    vector2d_add(r1.pos, ent->hurtbox.pos, ent->velocity);
    Uint8 col_data = 0;
    for (int i = 0; i < map_manager.maps[map_manager.current_map].rect_count; i++){
        Rect r2 = map_manager.maps[map_manager.current_map].collision[i];
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