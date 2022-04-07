#include "w_shuriken.h"
#include "simple_logger.h"
#include <math.h>
#include <stdio.h>

typedef struct {
    Uint32 time_remaining;
} ShurProps;

void shuriken_think(Entity *self){
    if(!self) return;
    ShurProps* properties = (ShurProps*)self->data;

    self->rotation.z += 5.0f;

    Entity* other = get_colliding(self);
    if(other != NULL && other != self->owner){
        other->health -= 8;
        entity_free(self);
        return;
    }

    if(properties->time_remaining == 0){
        vector2d_sub(self->velocity, self->owner->position, self->position);
        vector2d_normalize(&self->velocity);
        vector2d_set_magnitude(&self->velocity, 5);
        if(abs(self->owner->position.x - self->position.x) < 5 && abs(self->owner->position.y - self->position.y) < 5){
            entity_free(self);
        }
    } else {
        properties->time_remaining--;
    }
    //entity_free(self);
}


Entity* shuriken_ent_new(){
    Entity* ent = entity_new();
    ShurProps* k = (ShurProps*)ent->data;

    if(!ent){
        slog("no entities available");
    }

    ent->sprite = gf2d_sprite_load_all("images/shuriken.png",8,8,1);
    ent->think = shuriken_think;
    
    ent->velocity = vector2d(5, 0);

    ent->rotation = vector3d(4, 4, 0);

    ent->scale = vector2d(4,4);

    k->time_remaining = 60*2;
    ent->hurtbox = (Rect){{ent->position.x, ent->position.y}, {8*4, 8*4}};

    return ent;
}