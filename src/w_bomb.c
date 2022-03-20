#include "w_bomb.h"
#include "map.h"
#include "simple_logger.h"
#include <math.h>

typedef struct {
    Uint32 time_remaining;
    float radius;
} BombProps;

void bomb_think(Entity *self){
    if(!self) return;
    BombProps* properties = (BombProps*)self->data;
    properties->time_remaining--;

    Vector2D ground_probe;
    vector2d_add(ground_probe, self->position, vector2d(16, 32));
    Uint8 on_floor = collide_worldp(ground_probe);

    if(properties->time_remaining == 60*2){
        damage_radius(self->owner, self->position, 20, properties->radius);
    }

    if(properties->time_remaining < 60*2){
        self->frame += 0.2;
    }

    if(properties->time_remaining == 0){
        entity_free(self);
    }

    if(!on_floor){
        if(self->velocity.y < 4) self->velocity.y += 0.2;
    } else {
        self->velocity.y = -4;
    }

}


Entity* bomb_ent_new(){
    Entity* ent = entity_new();
    BombProps* b = (BombProps*)ent->data;

    if(!ent){
        slog("no entities available");
    }

    ent->sprite = gf2d_sprite_load_all("images/bomb.png",8,8,3);
    ent->think = bomb_think;
    
    b->time_remaining = 60*5; // 10 seconds till boom
    b->radius = 250.0f;

    ent->draw_offset.x = 8;
    ent->draw_offset.y = 8;
    ent->scale = vector2d(4,4);

    return ent;
}