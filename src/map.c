#include "map.h"
#include "camera.h"
#include "simple_json.h"
#include "simple_logger.h"
#include "gf2d_graphics.h"
#include "enemy.h"
#include "npc.h"
#include "audio.h"
#include "menu.h"
#include "pg_ui.h"
#include "interactable.h"

typedef struct {
    Uint8 editing;
    Map current_map;
    int should_warp;
    Warp warp_target;
    TextLine map_jsn_path;
    TextLine map_bgm;
} MapManager;

static MapManager map_manager = {0};
static int selected = -1;
static int selected_warp = -1;
static int dragging = 0;
static int px = 0, py = 0; 

int current_map_width(){
    return map_manager.current_map.map_width;
}

int current_map_height(){
    return map_manager.current_map.map_height;
}

void map_manager_play_bgm(){
    if(gfc_line_cmp(map_manager.map_bgm, "") != 0){
        audio_open_mod(map_manager.map_bgm);
        audio_play_mod();
    }
}

///
///
/// Map Processing (cleanup, load, new, update)
///
///

void map_cleanup(){
    if(map_manager.current_map.foreground != NULL){
        gf2d_sprite_free(map_manager.current_map.foreground);
    }
    if(map_manager.current_map.background != NULL){
        gf2d_sprite_free(map_manager.current_map.background);
    }
    if(map_manager.current_map.decoration != NULL){
        gf2d_sprite_free(map_manager.current_map.decoration);
    }
    
    memset(map_manager.current_map.collision, 0, sizeof(map_manager.current_map.collision));
    memset(map_manager.current_map.warps, 0, sizeof(map_manager.current_map.warps));
    map_manager.current_map.col_count = 0;
    map_manager.current_map.warp_count = 0;
}

int map_manager_get_warp_count(){
    return map_manager.current_map.warp_count;
}

void map_load(char* map_def){

    SJson* jsn;

    jsn = sj_load(map_def);
    
    if(!jsn){
        slog("Unable to open map def %s");
        return;
    }
    
    ui_manager_clear_ephemeral();

    gfc_line_cpy(map_manager.map_jsn_path, map_def);

    char* map_name = (char*)sj_get_string_value(sj_object_get_value(jsn, "name"));
    char* map_fg = (char*)sj_get_string_value(sj_object_get_value(jsn, "map_fg"));
    char* map_deco = (char*)sj_get_string_value(sj_object_get_value(jsn, "map_deco"));
    char* map_bg = (char*)sj_get_string_value(sj_object_get_value(jsn, "map_bg"));
    char* music_bg = (char*)sj_get_string_value(sj_object_get_value(jsn, "music_ambient"));

    if(music_bg != NULL){
        audio_open_mod(music_bg);
        gfc_line_cpy(map_manager.map_bgm, music_bg);
        audio_play_mod();
    }

    short is_town = 0;
    sj_get_bool_value(sj_object_get_value(jsn, "is_town"), &is_town);
    SJson* collision = sj_object_get_value(jsn, "collision");
    SJson* spawners = sj_object_get_value(jsn, "spawners");
    SJson* enemies = sj_object_get_value(jsn, "enemies");
    SJson* interactables = sj_object_get_value(jsn, "interactables");
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

    map_manager.current_map.col_count = sj_array_get_count(collision);
    if(map_manager.current_map.col_count > 80) map_manager.current_map.col_count = 80;

    for (int cr = 0; cr < map_manager.current_map.col_count; cr++){
        Rect* r = &map_manager.current_map.collision[cr];
        SJson* rect = sj_array_get_nth(collision, cr);
        int x, y, w, h;
        sj_get_integer_value(sj_array_get_nth(rect, 0), &x);
        sj_get_integer_value(sj_array_get_nth(rect, 1), &y);
        sj_get_integer_value(sj_array_get_nth(rect, 2), &w);
        sj_get_integer_value(sj_array_get_nth(rect, 3), &h);
        r->pos.x = (float)(x<<2);
        r->pos.y = (float)(y<<2);
        r->size.x = (float)(w<<2);
        r->size.y = (float)(h<<2);
    }

    map_manager.current_map.warp_count = sj_array_get_count(warps);
    if(map_manager.current_map.warp_count > 15) map_manager.current_map.warp_count = 15;
    for (int cr = 0; cr < map_manager.current_map.warp_count; cr++){
        SJson* warp = sj_array_get_nth(warps, cr);        
        Warp* nwarp = &map_manager.current_map.warps[cr];
        char* warp_path = (char*)sj_get_string_value(sj_object_get_value(warp, "map"));
        
        strncpy(nwarp->load_map, warp_path, sizeof(TextLine));

        SJson* dest = sj_object_get_value(warp, "dest_warp");
        if(!dest){
            nwarp->dest_warp = -1;
            //map_manager.current_map.warps[cr].dest_warp = -1;  
        } else {
            sj_get_integer_value(dest, &nwarp->dest_warp);
        }

        SJson* rect = sj_object_get_value(warp, "box");
        int x, y, w, h;
        sj_get_integer_value(sj_array_get_nth(rect, 0), &x);
        sj_get_integer_value(sj_array_get_nth(rect, 1), &y);
        sj_get_integer_value(sj_array_get_nth(rect, 2), &w);
        sj_get_integer_value(sj_array_get_nth(rect, 3), &h);

        SJson* locked = sj_object_get_value(warp, "locked");
        if(locked != NULL){
            sj_get_integer_value(locked, &nwarp->locked);
        }  else {
            nwarp->locked = 0;
        }

        nwarp->area = (Rect){{x << 2, y << 2}, {w << 2, h << 2}};
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
            case ENEMY_BAT:
                ent = bat_new();
                break;

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


    int npc_count = sj_array_get_count(npcs);
    
    for (int cr = 0; cr < npc_count; cr++){
        SJson* npc = sj_array_get_nth(npcs, cr);
        char* path = (char*)sj_get_string_value(npc);
        //return Entity*
        npc_new(path);
    }

    //spawn_weight_platforms(vector2d(960, 960), vector2d(1500, 960), 100);
    //spawn_unlock_door_button(vector2d(1300, 960), 1);
    //spawn_crate(vector2d(1400, 320), 0.5, 0);
    //spawn_timed_platforms_new(vector2d(1800, 360), 4);
    //spawn_timed_platforms(vector2d(1800, 360), 1000);

    int interactable_count = sj_array_get_count(interactables);
        
    for (int cr = 0; cr < interactable_count; cr++){
        SJson* interactable = sj_array_get_nth(interactables, cr);
        int type, x, y;

        sj_get_integer_value(sj_object_get_value(interactable, "type"), &type);

        SJson* position = sj_object_get_value(interactable, "position");
        sj_get_integer_value(sj_array_get_nth(position, 0), &x);
        sj_get_integer_value(sj_array_get_nth(position, 1), &y);

        Vector2D pos;
        pos.x = (float)(x << 2);
        pos.y = (float)(y << 2);

        switch (type){
            case CRATE:{
                int weight, craft_drop;
                sj_get_integer_value(sj_object_get_value(interactable, "weight"), &weight);
                sj_get_integer_value(sj_object_get_value(interactable, "drop"), &craft_drop);
                spawn_crate(pos, weight, craft_drop % CRAFTABLE_COUNT);
                break;
            }

            case SCALE_PLATFORM: {
                int max_dist;
                SJson* position = sj_object_get_value(interactable, "position_plat2");
                sj_get_integer_value(sj_array_get_nth(position, 0), &x);
                sj_get_integer_value(sj_array_get_nth(position, 1), &y);
                Vector2D pos2;
                pos2.x = (float)(x << 2);
                pos2.y = (float)(y << 2);
                sj_get_integer_value(sj_object_get_value(interactable, "max_drop"), &max_dist);
                spawn_weight_platforms(pos, pos2, max_dist);
                break;
            }

            case DOOR_BUTTON: {
                int warp_to_unlock;
                sj_get_integer_value(sj_object_get_value(interactable, "unlock"), &warp_to_unlock);
                spawn_unlock_door_button(pos, warp_to_unlock);
                break;
            }

            case TIMER_PLATFORM_BUTTON: {
                spawn_timed_platforms(pos, interactable);
                break;
            }
            
            case FALLING_PLATFORM:{
                int time_before_fall;
                sj_get_integer_value(sj_object_get_value(interactable, "time_before_fall"), &time_before_fall);
                spawn_falling_platform(pos, time_before_fall);
                break;
            }
        }
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

    map_editor_notify_load(music_bg, map_bg, map_fg, map_deco, map_def);

    map_manager.current_map.map_width = map_manager.current_map.background->frame_w * 4;
    map_manager.current_map.map_height = map_manager.current_map.background->frame_h * 4;

    sj_free(jsn);


}

void map_manager_unlock_warp(int warp){
    if(warp < map_manager.current_map.warp_count && map_manager.current_map.warps[warp].locked){
        map_manager.current_map.warps[warp].locked = 0;
    }
}

void map_new(){
    //what? how?
}

void map_save(char* path){
    int i;
    SJson* cur_warp, *cur_rect, *collision, *warps;
    SJson* map = sj_object_new();
    sj_object_insert(map, "name", sj_new_str(map_manager.current_map.name));
    if(map_manager.current_map.foreground != NULL){
        sj_object_insert(map, "map_fg", sj_new_str(map_manager.current_map.foreground->filepath));
    }
    if(map_manager.current_map.decoration != NULL){
        sj_object_insert(map, "map_deco", sj_new_str(map_manager.current_map.decoration->filepath));
    }
    if(map_manager.current_map.background != NULL){
        sj_object_insert(map, "map_bg", sj_new_str(map_manager.current_map.background->filepath));
    }
    
    sj_object_insert(map, "music_ambient", sj_new_str(map_manager.map_bgm));

    sj_object_insert(map, "is_town", sj_new_int(map_manager.current_map.is_town));

    collision = sj_array_new();
    for(i=0; i < map_manager.current_map.col_count; i++){
        Rect* r = &map_manager.current_map.collision[i];
        cur_rect = sj_array_new();
        sj_array_append(cur_rect, sj_new_int(((int)r->pos.x) >> 2));
        sj_array_append(cur_rect, sj_new_int(((int)r->pos.y) >> 2));
        sj_array_append(cur_rect, sj_new_int(((int)r->size.x) >> 2));
        sj_array_append(cur_rect, sj_new_int(((int)r->size.y) >> 2));
        sj_array_append(collision, cur_rect);
        
    }
    sj_object_insert(map, "collision", collision);

    warps = sj_array_new();
    for (i = 0; i < map_manager.current_map.warp_count; i++){
        Warp* r = &map_manager.current_map.warps[i];
        cur_warp = sj_object_new();
        cur_rect = sj_array_new();
        
        sj_array_append(cur_rect, sj_new_int(((int)r->area.pos.x) >> 2));
        sj_array_append(cur_rect, sj_new_int(((int)r->area.pos.y) >> 2));
        sj_array_append(cur_rect, sj_new_int(((int)r->area.size.x) >> 2));
        sj_array_append(cur_rect, sj_new_int(((int)r->area.size.y) >> 2));
    
        sj_object_insert(cur_warp, "map", sj_new_str(r->load_map));
        sj_object_insert(cur_warp, "box", cur_rect);
        sj_object_insert(cur_warp, "dest_warp", sj_new_int(r->dest_warp));
        sj_object_insert(cur_warp, "locked", sj_new_int(r->locked));
        sj_array_append(warps, cur_warp);
    }
    sj_object_insert(map, "warps", warps);
    
    entity_manager_serialize(map);

    sj_save(map, path);
    sj_free(map);
}

void map_manager_update(){
    int i, x, y;
    Uint32 state;
    Vector2D mouse_pos;
    if(map_manager.editing == 0){
        if(map_manager.should_warp == 1){
            entity_manager_clear(entity_manager_get_player());
            map_load(map_manager.warp_target.load_map);
            map_editor_notify_selected(NULL);
            selected = -1;
            if(map_manager.warp_target.dest_warp != -1){
                Warp* wrp = &map_manager.current_map.warps[map_manager.warp_target.dest_warp];//gfc_list_get_nth(map_manager.current_map.warps, map_manager.warp_target->dest_warp);
                entity_manager_get_player()->position = wrp->area.pos;
                map_manager.current_map.player_spawn = wrp->area.pos;
            }
            map_manager.should_warp = 0;
        }

    } else {
        state = SDL_GetMouseState(&x, &y);
        mouse_pos = vector2d(x, y);
        vector2d_add(mouse_pos, get_camera_pos(), mouse_pos);

        if((state & SDL_BUTTON_RMASK) != 0){
            map_editor_notify_selected(NULL);
            map_editor_notify_selected_warp(NULL);
            selected_warp = -1;
            dragging = 0;
        }

        if((state & SDL_BUTTON_LMASK) != 0 && dragging == 0){
            for(i=0; i < map_manager.current_map.col_count; i++){
                Rect* r = &map_manager.current_map.collision[i];//gfc_list_get_nth(map_manager.current_map.collision, i);
                if(rectp_collidep(mouse_pos, r)){
                    map_editor_notify_selected(r);
                    selected = i;
                    dragging = 1;
                    px = x;
                    py = y;
                    break;
                }
            }

            for (i = 0; i < map_manager.current_map.warp_count; i++){
                Warp* r = &map_manager.current_map.warps[i];
                if(rect_collidep(mouse_pos, r->area)){
                    map_editor_notify_selected_warp(r);
                    selected_warp = i;
                    break;
                }
            }
        } else if ((state & SDL_BUTTON_LMASK) != 0 && dragging == 1){
            Rect* r = &map_manager.current_map.collision[selected];
            r->pos.x += x - px;
            r->pos.y += y - py;
            px = x;
            py = y;
        } else if ((state & SDL_BUTTON_LMASK) == 0 && dragging == 1){
            dragging = 0;
        }
    }
}

void try_warp(Rect r){
    for (size_t i = 0; i < map_manager.current_map.warp_count; i++){
        Warp* wrp = &map_manager.current_map.warps[i];//gfc_list_get_nth(map_manager.current_map.warps, i);
        if(rect_collider(wrp->area, r) && wrp->locked == 0){
            map_manager.should_warp = 1;
            memcpy(&map_manager.warp_target, wrp, sizeof(Warp));
            break;
        }
    }
}

///
///
/// Map Draw Functions
///
///

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
    
    if(map_manager.editing == 1){
        Vector2D cam = get_camera_pos();
        for (size_t i = 0; i < map_manager.current_map.col_count; i++){
            if(i == selected){
                SDL_SetRenderDrawColor(gf2d_graphics_get_renderer(), 0xFF, 0xFF, 0xFF, 0xFF);
            } else {
                SDL_SetRenderDrawColor(gf2d_graphics_get_renderer(), 0xFF, 0x00, 0xFF, 0xFF);
            }
            SDL_FRect drect;
            Rect* r = &map_manager.current_map.collision[i];//gfc_list_get_nth(map_manager.current_map.collision, i);
            drect.x = r->pos.x - cam.x;
            drect.y = r->pos.y - cam.y;
            drect.w = r->size.x;
            drect.h = r->size.y;
            SDL_RenderDrawRectF(gf2d_graphics_get_renderer(), &drect);
        }
        for (size_t i = 0; i < map_manager.current_map.warp_count; i++){
            if(i == selected_warp){
                SDL_SetRenderDrawColor(gf2d_graphics_get_renderer(), 0xFF, 0xAA, 0xAA, 0xFF);
            } else {
                SDL_SetRenderDrawColor(gf2d_graphics_get_renderer(), 0x00, 0xAA, 0xFF, 0xFF);
            }
            SDL_FRect drect;
            Warp* r = &map_manager.current_map.warps[i];//gfc_list_get_nth(map_manager.current_map.warps, i);
            drect.x = r->area.pos.x - cam.x;
            drect.y = r->area.pos.y - cam.y;
            drect.w = r->area.size.x;
            drect.h = r->area.size.y;
            SDL_RenderDrawRectF(gf2d_graphics_get_renderer(), &drect);
        }
    }
}

void map_manager_draw_bg(){
    map_draw_bg(&map_manager.current_map);
}

///
///
/// Map Editor Notify Functions
///
///

void map_manager_notify_editing(Uint8 is_editing){
    map_manager.editing = is_editing;
}

void map_editor_notify_delete(){
    memset(&map_manager.current_map.collision[selected], 0, sizeof(Rect));
    if(selected_warp == map_manager.current_map.col_count - 1){
        map_manager.current_map.col_count--;
        return;
    }

    memmove(&map_manager.current_map.collision[selected], &map_manager.current_map.collision[selected+1], sizeof(Rect)*(map_manager.current_map.col_count - selected));
    map_manager.current_map.col_count--;
}

void map_editor_notify_add(Rect to_add){
    if(map_manager.current_map.col_count + 1 >= 50){
        return;
    }
    memcpy(&map_manager.current_map.collision[map_manager.current_map.col_count], &to_add, sizeof(Rect));
    map_manager.current_map.col_count++;
}

void map_editor_notify_delete_warp(){
    memset(&map_manager.current_map.warps[selected_warp], 0, sizeof(Warp));
    if(selected_warp == map_manager.current_map.warp_count - 1){
        map_manager.current_map.warp_count--;
        return;
    }

    memmove(&map_manager.current_map.warps[selected_warp], &map_manager.current_map.warps[selected_warp+1], sizeof(Warp)*(map_manager.current_map.warp_count - selected_warp));
    map_manager.current_map.warp_count--;
}

void map_editor_notify_add_warp(Warp to_add){
    if(map_manager.current_map.warp_count + 1 >= 15){
        return;
    }
    memcpy(&map_manager.current_map.warps[map_manager.current_map.warp_count], &to_add, sizeof(Warp));
    map_manager.current_map.warp_count++;
}

void map_editor_notify_set_bgm(char* path){
    gfc_line_cpy(map_manager.map_bgm, path);
    audio_open_mod(path);
}


void map_editor_notify_set_bg(char* path){
    if(map_manager.current_map.background != NULL){
        gf2d_sprite_free(map_manager.current_map.background);
    }
    map_manager.current_map.background = gf2d_sprite_load_image(path);
    map_manager.current_map.map_width = map_manager.current_map.background->frame_w << 2;
    map_manager.current_map.map_height = map_manager.current_map.background->frame_h << 2;
}

void map_editor_notify_set_fg(char* path){
    if(map_manager.current_map.foreground != NULL){
        gf2d_sprite_free(map_manager.current_map.foreground);
    }
    map_manager.current_map.foreground = gf2d_sprite_load_image(path);    
}

void map_editor_notify_set_deco(char* path){
    if(map_manager.current_map.decoration != NULL){
        gf2d_sprite_free(map_manager.current_map.decoration);
    }
    map_manager.current_map.decoration = gf2d_sprite_load_image(path);
}

void map_editor_notify_set_dimensions(Vector2D dimensions){
    map_manager.current_map.map_width = dimensions.x;
    map_manager.current_map.map_height = dimensions.y;
}

///
///
/// Collision Functions
///
///


Uint8 collide_worldp(Vector2D p){
    for (int i = 0; i < map_manager.current_map.col_count; i++){
        Rect* r = &map_manager.current_map.collision[i];
        if(rectp_collidep(p, r)){
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

    for (int i = 0; i < map_manager.current_map.col_count; i++){
        Rect* r2 = &map_manager.current_map.collision[i];
        Uint8 col_xl = r1.pos.x <= r2->pos.x + r2->size.x;
        Uint8 col_xr = r1.pos.x + r1.size.x >= r2->pos.x;

        Uint8 col_yu = r1.pos.y <= r2->pos.y + r2->size.y;
        Uint8 col_yd = r1.pos.y + r1.size.y >= r2->pos.y;

        Uint8 col_fyu = r1.pos.y + (r1.size.y * 0.2) < r2->pos.y + r2->size.y;
        Uint8 col_fyd = r1.pos.y + (r1.size.y * 0.8) > r2->pos.y;

        if(col_xl && col_xr && col_fyd && col_fyu){
            if(ent->velocity.x > 0){
                ent->position.x = r2->pos.x - r1.size.x - 3;
            } else if(ent->velocity.x < 0) {
                ent->position.x = r2->pos.x + r2->size.x + 3;
            }
            ent->velocity.x = 0;
        }

        if(col_xl && col_xr && col_yd && col_yu){
            if(col_yd && ent->velocity.y >= 0){
                ent->velocity.y = 0;
                if(!col_fyd) ent->position.y = r2->pos.y - r1.size.y;
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