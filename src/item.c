#include "item.h"
#include "map.h"

void potion_think(Entity* self){
    Entity* player = entity_manager_get_player();
    PickupProps* props = (PickupProps*)self->data;

    if(self->velocity.y > 0){
        Uint8 on_floor = ent_collide_world(self);
        if(on_floor & COL_FLOOR) self->velocity.y = 0;
    }

    props->lifetime--;
    if(props->lifetime == 0) entity_free(self);
    if(rect_collider(self->hurtbox, player->hurtbox)){
        inventory_add_consumable(props->type, props->count);
        entity_free(self);
    }
}

Entity* spawn_potion(ConsumableType type){
    Entity* ent = entity_new();
    PickupProps* props = (PickupProps*)ent->data;
    if(!ent) return NULL;

    ent->sprite = inventory_manager_consumables_img();
    ent->scale = vector2d(0.25, 0.25);
    ent->velocity.y = 2;
    ent->think = potion_think;
    ent->hurtbox = (Rect){0, 0, 32, 32};

    props->lifetime = 300;
    props->count = 1;
    props->type = type;
    ent->frame = type;
    return ent;
}