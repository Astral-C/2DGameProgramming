#include "simple_logger.h"
#include "pg_entity.h"
#include "camera.h"
#include "gf2d_graphics.h"

#define DRAW_DEBUG true

typedef struct {
    Uint32 max_entities;
    Entity *entity_list;
    Entity* player;
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

    entity_manager.max_entities = max_entities;
    entity_manager.entity_list = gfc_allocate_array(sizeof(Entity),max_entities);
    atexit(entity_manager_close);
}

Entity* entity_new(){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(!entity_manager.entity_list[i]._inuse){
            entity_manager.entity_list[i]._inuse = 1;
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
    vector2d_add(drawpos, ent->position, ent->draw_offset);
    vector2d_sub(drawpos, drawpos, cam_pos);
    gf2d_sprite_draw(ent->sprite, drawpos, &ent->scale, NULL, &ent->rotation, &ent->flip, NULL, (Uint32)ent->frame);

    if(DRAW_DEBUG && gf2d_graphics_get_renderer()){
        SDL_FRect db;
        Rect hbox = ent->hurtbox;
        db.x = hbox.pos.x - cam_pos.x;
        db.y = hbox.pos.y - cam_pos.y;
        db.w = hbox.size.x;
        db.h = hbox.size.y;
        SDL_SetRenderDrawColor(gf2d_graphics_get_renderer(), 0xFF, 0x00, 0xFF, 0xFF);
        SDL_RenderDrawRectF(gf2d_graphics_get_renderer(), &db);
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
        if(entity_manager.entity_list[i]._inuse){
            entity_manager.entity_list[i].think(&entity_manager.entity_list[i]);
            vector2d_add(entity_manager.entity_list[i].position, entity_manager.entity_list[i].velocity, entity_manager.entity_list[i].position);
            vector2d_copy(entity_manager.entity_list[i].hurtbox.pos, entity_manager.entity_list[i].position);
        }
    }
}

void entity_manager_close(){
    entity_manager_clear();

    if(entity_manager.entity_list != NULL){
        free(entity_manager.entity_list);
    }
}

void entity_manager_clear(){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(!entity_manager.entity_list[i]._inuse) continue;
        entity_free(&entity_manager.entity_list[i]);
    }
}

void entity_manager_set_player(Entity* e){
    entity_manager.player = e;
}

Entity* entity_manager_get_player(){
    return entity_manager.player;
}

void entity_free(Entity* ent){
    if(ent == NULL){
        return;
    }
    
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

void damage_radius(Entity* ignore, Vector2D p, Uint8 damage, float range){
    int i;
    for(i=0;i<entity_manager.max_entities;i++){
        if(entity_manager.entity_list[i]._inuse && &entity_manager.entity_list[i] != ignore && abs(p.x - entity_manager.entity_list[i].position.x) < range && abs(p.y - entity_manager.entity_list[i].position.y)) {
            damage_default(NULL, &entity_manager.entity_list[i], damage);
        }
    }
}