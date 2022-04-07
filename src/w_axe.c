#include "w_axe.h"
#include "simple_logger.h"
#include <math.h>
#include <stdio.h>

typedef struct {
    Uint32 time_remaining;
} AxeProps;

void axe_think(Entity *self){
    if(!self) return;
    AxeProps* properties = (AxeProps*)self->data;

    self->frame += 0.1f;
    if(self->frame > 4.0f) self->frame = 0.0f;
    self->velocity.y = fmin(5.0f, self->velocity.y+0.4f);

   Entity* hit = get_colliding(self);

    if(hit != NULL && hit != self->owner){
        hit->health -= 20;
        entity_free(self);
        return;
    }

   properties->time_remaining--;
   if(properties->time_remaining == 0){
       entity_free(self);
   }

}


Entity* axe_ent_new(){
    Entity* ent = entity_new();
    AxeProps* k = (AxeProps*)ent->data;

    if(!ent){
        slog("no entities available");
    }

    ent->sprite = gf2d_sprite_load_all("images/axe.png",16,16,4);
    ent->think = axe_think;
    
    ent->velocity = vector2d(6, -10);

    ent->scale = vector2d(4,4);

    k->time_remaining = 360;
    ent->hurtbox = (Rect){{ent->position.x, ent->position.y}, {64, 64}};

    return ent;
}