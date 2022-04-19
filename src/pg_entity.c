#include "simple_logger.h"
#include "pg_entity.h"
#include "camera.h"
#include "gf2d_graphics.h"

#define DRAW_DEBUG false

typedef struct {
    Uint8 frozen;
    Uint32 max_entities;
    Entity *entity_list;
    Entity* player;
    Entity* selected; // used for the editor only
    Uint8 dragging;
    Uint8 draw_debug;
} EntityManager;

static EntityManager entity_manager = {0};

void entity_manager_init(Uint32 max_entities){
    if(max_entities == 0){
        slog("Cannot allocate memory for 0 entities");
        return;
    }

    if(entity_manager.entity_list != NULL){
        slog("Entity manager already intitialized");
        return;
    }
    entity_manager.draw_debug = DRAW_DEBUG;
    entity_manager.max_entities = max_entities;
    entity_manager.entity_list = gfc_allocate_array(sizeof(Entity),max_entities);
    atexit(entity_manager_close);
}

Entity* entity_new(){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(!entity_manager.entity_list[i]._inuse){
            entity_manager.entity_list[i]._inuse = 1;
            entity_manager.entity_list[i].visible = 1;
            return &entity_manager.entity_list[i];
        }
    }

    slog("All entities in use");
    return NULL;
}

void draw_entity(Entity* ent){
    if(ent == NULL) return;
    if(ent->sprite == NULL) return;
    Vector2D drawpos;
    Vector2D cam_pos = get_camera_pos();
    Vector4D color_overlay;
    
    if(ent->type == ENT_PLAYER){
        color_overlay = vector4d(255, 255, 255, (ent->visible ? 255 : 255/2));
    } else {
        if(!entity_manager.draw_debug) color_overlay = vector4d(255, 255, 255, (ent->visible ? 255 : 0));
    }

    vector2d_add(drawpos, ent->position, ent->draw_offset);
    vector2d_sub(drawpos, drawpos, cam_pos);
    gf2d_sprite_draw(ent->sprite, drawpos, &ent->scale, NULL, &ent->rotation, &ent->flip, &color_overlay, (Uint32)ent->frame);

    if(entity_manager.draw_debug && gf2d_graphics_get_renderer()){
        SDL_FRect db;
        Rect hbox = ent->hurtbox;
        db.x = hbox.pos.x - cam_pos.x;
        db.y = hbox.pos.y - cam_pos.y;
        db.w = hbox.size.x;
        db.h = hbox.size.y;
        SDL_SetRenderDrawColor(gf2d_graphics_get_renderer(), 0xFF, 0x00, 0xFF, 0xFF);
        SDL_RenderDrawRectF(gf2d_graphics_get_renderer(), &db);
        if(ent == entity_manager.player){
            SDL_FRect whip_hb = db; 
            whip_hb.x += 32;
            whip_hb.y += 32;
            whip_hb.h = 16;
            SDL_RenderDrawRectF(gf2d_graphics_get_renderer(), &whip_hb);
        }
    }
}

void entity_manager_draw_entities(){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(entity_manager.entity_list[i]._inuse){
            draw_entity(&entity_manager.entity_list[i]);
        }
    }
}

void entity_manager_think_all(){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if((entity_manager.entity_list[i]._inuse && !entity_manager.frozen) || &entity_manager.entity_list[i] == entity_manager.player){
            entity_manager.entity_list[i].think(&entity_manager.entity_list[i]);
            vector2d_add(entity_manager.entity_list[i].position, entity_manager.entity_list[i].velocity, entity_manager.entity_list[i].position);
            vector2d_copy(entity_manager.entity_list[i].hurtbox.pos, entity_manager.entity_list[i].position);
        }
    }
}

void entity_manager_think_edit(){
    int i, x, y;
    Uint32 state;
    Vector2D mouse_pos, offset;
    state = SDL_GetMouseState(&x, &y);
    mouse_pos = vector2d(x, y);
    vector2d_add(mouse_pos, get_camera_pos(), mouse_pos);
    if(state & SDL_BUTTON_LMASK != 0){
        for(i=0;i<entity_manager.max_entities;i++){
            if(entity_manager.entity_list[i]._inuse){
                if(rect_collidep(mouse_pos, entity_manager.entity_list[i].hurtbox) && entity_manager.dragging == 0){
                    entity_manager.selected = &entity_manager.entity_list[i];
                    entity_manager.dragging = 1;
                }
            }
        }
    } else if(entity_manager.dragging == 1) {
        entity_manager.dragging = 0;
    }

    if(entity_manager.selected != NULL && entity_manager.dragging){
        vector2d_scale(offset, entity_manager.selected->hurtbox.size, 0.5);
        vector2d_sub(entity_manager.selected->position, mouse_pos, offset);
        entity_manager.selected->hurtbox.pos = entity_manager.selected->position;
    }
}

void entity_manager_close(){
    entity_manager_clear(NULL);

    if(entity_manager.entity_list != NULL){
        free(entity_manager.entity_list);
    }
}

void entity_manager_clear(Entity* filter){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(!entity_manager.entity_list[i]._inuse) continue;
        if(&entity_manager.entity_list[i] == filter) continue;
        entity_free(&entity_manager.entity_list[i]);
    }
}

void entity_manager_reset_player(){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(entity_manager.entity_list[i]._inuse && entity_manager.entity_list[i].type == ENT_PLAYER){
            entity_manager.player = &entity_manager.entity_list[i];
            return;
        }
    }
}

Entity* entity_manager_get_player(){
    return entity_manager.player;
}

Entity* entity_manager_get_selected(){
    return entity_manager.selected;
}

void entity_free(Entity* ent){
    if(ent == NULL){
        return;
    }
    if(ent->cleanup) ent->cleanup(ent);
    if(ent->sprite != NULL){
        gf2d_sprite_free(ent->sprite);
    }

    memset(ent, 0, sizeof(Entity));
}

void damage_default(Entity* self, Entity* other, Uint8 damage){
    other->health -= damage;
}

Entity* get_colliding(Entity* ent){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(entity_manager.entity_list[i]._inuse){ //add collision masks at some point? idfk at this point
            if(ent != &entity_manager.entity_list[i] && rect_collider(ent->hurtbox, entity_manager.entity_list[i].hurtbox)){
                return &entity_manager.entity_list[i];
            }
        }
    }
    return NULL;
}

Entity* get_colliding_r(Rect r, Entity* filter){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(&entity_manager.entity_list[i] != filter && entity_manager.entity_list[i]._inuse){ //add collision masks at some point? idfk at this point
            if(rect_collider(r, entity_manager.entity_list[i].hurtbox)){
                return &entity_manager.entity_list[i];
            }
        }
    }
    return NULL;
}

void damage_radius(Entity* ignore, Vector2D p, Uint8 damage, float range){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(entity_manager.entity_list[i]._inuse && &entity_manager.entity_list[i] != ignore && abs(p.x - entity_manager.entity_list[i].position.x) < range && abs(p.y - entity_manager.entity_list[i].position.y)) {
            damage_default(NULL, &entity_manager.entity_list[i], damage);
        }
    }
}

void entity_manager_time_freeze(){
    entity_manager.frozen = 1;
}

void entity_manager_time_unfreeze(){
    entity_manager.frozen = 0;
}

void entity_manager_toggle_draw_debug(){
    entity_manager.draw_debug = !entity_manager.draw_debug;
}

void entity_manager_set_draw_debug(Uint8 draw){
    entity_manager.draw_debug = draw;
}

void entity_manager_kill_enemies(){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(entity_manager.entity_list[i]._inuse && entity_manager.entity_list[i].type == ENT_ENEMY){ //add collision masks at some point? idfk at this point
            entity_manager.entity_list[i].health = 0;
        }
    }
}