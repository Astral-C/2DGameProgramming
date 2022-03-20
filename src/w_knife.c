#include "w_knife.h"
#include "simple_logger.h"
#include <math.h>

#define WEP_DMG 5

typedef struct {
    Uint32 time_remaining;
    int dmg;
} KnifeProps;

void knife_think(Entity *self){
    if(!self) return;
    KnifeProps* properties = (KnifeProps*)self->data;

    Entity* other = get_colliding(self);
    if(other != NULL && other != self->owner){
        other->health -= properties->dmg;
        entity_free(self);
    }
}


Entity* knife_ent_new(){
    Entity* ent = entity_new();
    KnifeProps* k = (KnifeProps*)ent->data;

    if(!ent){
        slog("no entities available");
    }

    ent->sprite = gf2d_sprite_load_all("images/knife.png",8,8,1);
    ent->think = knife_think;
    
    ent->velocity = vector2d(6, 0);
    k->dmg = 15;

    ent->draw_offset.x = -4;
    ent->draw_offset.y = -4;
    ent->scale = vector2d(4,4);
    ent->hurtbox = (Rect){ent->position.x, ent->position.y, 8*4, 8*2};

    return ent;
}