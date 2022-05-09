#include "interactable.h"
#include "simple_json.h"
#include "gfc_vector.h"
#include "inventory.h"
#include "gfc_input.h"
#include "player.h"
#include "item.h"
#include "map.h"

//////////////////////////////////////////////
///
/// Interactable Think Functions
///
//////////////////////////////////////////////

void timed_platform_think(Entity* self){
    if(self->owner == NULL) return; //what?
    TimedPlatformData* platform_data = (TimedPlatformData*)self->data;
    TimePlatformButtonData* linked_button = (TimePlatformButtonData*)self->owner->data;
    
    if(!platform_data->enabled && linked_button->activated){
        platform_data->enabled = 1;
        self->visible = 1;
    }

    if(platform_data->enabled && !linked_button->activated){
        platform_data->enabled = 0;
        self->visible = 0;
    }

    if(!platform_data->enabled){
        return;
    }
    
    if(platform_data->move){
        platform_data->x += 0.05;
        self->position.y = platform_data->start_y + platform_data->amplitude * sin(platform_data->x + platform_data->offset);
    }

    Entity* player = entity_manager_get_player();
    if(rect_collider(self->hurtbox, player->hurtbox)){
        *((Uint8*)player->data) = 1;
        player->velocity.y = 0;
        player->position.y = self->position.y - (player->hurtbox.size.y - 8);
    }

}

void timed_platform_button_think(Entity* self){
    TimePlatformButtonData* button = (TimePlatformButtonData*)self->data;
    if(button->activated){
        button->time_remaining--; 
        if(button->time_remaining == 0){
            button->activated = 0;
            button->time_remaining = button->time_max;
            self->frame = 0;
        }
        return;
    }

    Entity* player = entity_manager_get_player();
    if(rect_collider(self->hurtbox, player->hurtbox)){
        button->activated = 1;
        self->frame = 1;
    }

}

void crate_think(Entity* self){
    Entity* player = entity_manager_get_player();
    CrateData* crate = (CrateData*)self->data; 
    
    if(self->health <= 0){
        Entity* ci = spawn_craft_item(crate->drop);
        vector2d_copy(ci->position, self->position);
        entity_free(self);
    }

    if(!ent_collide_world(self)){
        self->velocity.y = 2.0 * 1/crate->weight;
    }


    if(player->velocity.x != 0 && player->velocity.y == 0 && abs(player->position.y - self->position.y) < 0.1 && rect_collider(self->hurtbox, player->hurtbox)){
        self->velocity.x = player->velocity.x * crate->weight;
        if(player->velocity.x > 0) player->position.x = self->position.x - 64;
        if(player->velocity.x < 0) player->position.x = self->position.x + self->hurtbox.size.x;
    } else if(player->velocity.y != 0 && rect_collider(self->hurtbox, player->hurtbox)) {
        player->velocity.y = 1.0;
        player->position.y = self->position.y - (player->hurtbox.size.y - 2);
        *((Uint8*)player->data) = 1;
    } else {
        self->velocity.x = ( self->velocity.x > 0 ? self->velocity.x - 1 : (self->velocity.x < 0 ? self->velocity.x + 1 : 0));
    }
}

void door_button_think(Entity* self){
    Entity* player = entity_manager_get_player();
    WarpUnlockData* button = (WarpUnlockData*)self->data; 
    if(rect_collider(self->hurtbox, player->hurtbox) && gfc_input_command_released("whip")){
        self->frame = 1;
        map_manager_unlock_warp(button->warp);
    }
}

void weight_platform_think(Entity* self){
    Entity* player = entity_manager_get_player();
    WeightedPlatformData* d = (WeightedPlatformData*)self->data; 
    if(rect_collider(self->hurtbox, player->hurtbox) && player->velocity.y > -1){

        *((Uint8*)player->data) = 1;
        player->velocity.y = 0;
        player->position.y = self->position.y - (player->hurtbox.size.y - 8);
        self->velocity.y = 2.0;
    } else {
        self->velocity.y = 0.0;
    }

    if(self->owner->velocity.y > 0){
        self->velocity.y = -self->owner->velocity.y;
    }
    
    if(self->position.y < d->start_y - d->max_drop && self->velocity.y < 0){
        self->velocity.y = 0;
    }

    if(self->position.y > d->start_y + d->max_drop && self->velocity.y > 0){
        self->velocity.y = 0;
    }
}

void falling_platform_think(Entity* self){
    Entity* player = entity_manager_get_player();
    FallingPlatformData* d = (FallingPlatformData*)self->data; 
    if(rect_collider(self->hurtbox, player->hurtbox) && player->velocity.y > -1){

        *((Uint8*)player->data) = 1;
        player->velocity.y = 0;
        player->position.y = self->position.y - (player->hurtbox.size.y - 8);
        d->should_fall = 1;
    }

    if(d->should_fall){
        if(d->time_before_fall <= 0){
            self->velocity.y = 6.0;
        } else {
            d->time_before_fall--;
        }
    }

    if(abs(d->start_y - self->position.y) > 400){
        entity_free(self);
    }
    
}

//////////////////////////////////////////////
///
/// Spawn Functions for interactables
///
//////////////////////////////////////////////

Entity* spawn_unlock_door_button(Vector2D pos, int warp){
    Entity* e = entity_new();
    e->type = ENT_INTERACTABLE;
    e->think = door_button_think;
    vector2d_copy(e->position, pos);
    e->hurtbox = (Rect){{pos.x, pos.y},{36, 36}};
    e->sprite = gf2d_sprite_load_all("images/button.png", 36, 36, 2);
    e->scale = vector2d(1,1);
    e->draw_offset = vector2d(0,0);

    WarpUnlockData* button = (WarpUnlockData*)e->data;
    button->warp = warp;
    button->type = DOOR_BUTTON;
    return e;
}

Entity* spawn_weight_platforms(Vector2D platform1, Vector2D platform2, float max_drop){
    Entity* p1 = entity_new();
    p1->type = ENT_INTERACTABLE;
    p1->think = weight_platform_think;
    vector2d_copy(p1->position, platform1);
    p1->velocity = vector2d(0,0);
    p1->hurtbox = (Rect){{p1->position.x, p1->position.y},{64, 32}};

    WeightedPlatformData* d1 = (WeightedPlatformData*)p1->data;
    d1->type = SCALE_PLATFORM;
    d1->start_y = platform1.y;
    d1->max_drop = max_drop;
    p1->sprite = gf2d_sprite_load_image("images/drop_platform.png");
    p1->scale = vector2d(1,1);
    p1->draw_offset = vector2d(0,0);


    Entity* p2 = entity_new();
    p2->type = ENT_INTERACTABLE;
    p2->think = weight_platform_think;
    vector2d_copy(p2->position, platform2);
    p2->velocity = vector2d(0,0);
    p2->hurtbox = (Rect){{p2->position.x, p2->position.y},{64, 32}};

    WeightedPlatformData* d2 = (WeightedPlatformData*)p2->data;
    d2->type = -1; //ephemeral, should _not_ be considered when writing!
    d2->start_y = platform2.y;
    d2->max_drop = max_drop;
    p2->sprite = gf2d_sprite_load_image("images/drop_platform.png");
    p2->scale = vector2d(1,1);
    p2->draw_offset = vector2d(0,0);

    p1->owner = p2;
    p2->owner = p1;
    return p1;
}

Entity* spawn_crate(Vector2D pos, float weight, CraftType drop){
    Entity* e = entity_new();
    e->type = ENT_INTERACTABLE;
    e->think = crate_think;
    vector2d_copy(e->position, pos);
    e->sprite = gf2d_sprite_load_image("images/crate.png");
    e->hurtbox = (Rect){{pos.x, pos.y},{e->sprite->frame_w, e->sprite->frame_h}};
    e->scale = vector2d(1,1);
    e->draw_offset = vector2d(0,0);
    e->health = 50;

    CrateData* crate = (CrateData*)e->data;
    crate->weight = weight;
    crate->drop = drop; //drops craftable if it falls too far
    crate->type = CRATE;

    return e;   
}

Entity* spawn_timed_platform(Entity* parent, SJson* platform_json){
    Vector2D pos;
    int x = 0;
    int y = 0;

    SJson* pos_json = sj_object_get_value(platform_json, "position");
    sj_get_integer_value(sj_array_get_nth(pos_json, 0), &x);
    sj_get_integer_value(sj_array_get_nth(pos_json, 1), &y);
    pos.x = (float)(x << 2);
    pos.y = (float)(y << 2);

    Entity* e = entity_new();
    e->type = ENT_INTERACTABLE;
    e->think = timed_platform_think;
    
    vector2d_copy(e->position, pos);
    e->sprite = gf2d_sprite_load_image("images/drop_platform.png");
    e->hurtbox = (Rect){{pos.x, pos.y},{e->sprite->frame_w, e->sprite->frame_h}};
    e->scale = vector2d(1,1);
    e->draw_offset = vector2d(0,0);
    e->owner = parent;
    e->visible = 0;

    TimedPlatformData* platform = (TimedPlatformData*)e->data;
    platform->type = TIMER_PLATFORM;
    platform->start_y = pos.y;

    sj_get_integer_value(sj_object_get_value(platform_json, "should_float"), &platform->move);
    sj_get_integer_value(sj_object_get_value(platform_json, "float_amplitude"), &platform->amplitude);
    sj_get_float_value(sj_object_get_value(platform_json, "float_offset"), &platform->offset);

    printf("Spawning timer platform at pos %f %f\n", e->position.x, e->position.y);

    return e;
}


Entity* spawn_timed_platforms(Vector2D pos, SJson* timed_platforms){

    Entity* e = entity_new();
    e->type = ENT_INTERACTABLE;
    e->think = timed_platform_button_think;
    vector2d_copy(e->position, pos);
    e->sprite = gf2d_sprite_load_all("images/timer_button.png", 64, 32, 2);
    e->hurtbox = (Rect){{pos.x, pos.y},{e->sprite->frame_w, e->sprite->frame_h}};
    e->scale = vector2d(1,1);
    e->draw_offset = vector2d(0,0);

    TimePlatformButtonData* button = (TimePlatformButtonData*)e->data;
    button->type = TIMER_PLATFORM_BUTTON;
    button->activated = 0;
    sj_get_integer_value(sj_object_get_value(timed_platforms, "max_time"), &button->time_max);
    button->time_remaining = button->time_max;

    SJson* platforms = sj_object_get_value(timed_platforms, "platforms");
    int platform_count = sj_array_get_count(platforms);
    for (int i = 0; i < platform_count; i++){
        spawn_timed_platform(e, sj_array_get_nth(platforms, i));
    }
    

    return e;   
}



Entity* spawn_timed_platform_new(Entity* parent, Vector2D pos){
    Entity* e = entity_new();
    e->type = ENT_INTERACTABLE;
    e->think = timed_platform_think;

    vector2d_copy(e->position, pos);
    e->sprite = gf2d_sprite_load_image("images/drop_platform.png");
    e->hurtbox = (Rect){{pos.x, pos.y},{e->sprite->frame_w, e->sprite->frame_h}};
    e->scale = vector2d(1,1);
    e->draw_offset = vector2d(0,0);
    e->owner = parent;
    e->visible = 0;

    TimedPlatformData* platform = (TimedPlatformData*)e->data;
    platform->type = TIMER_PLATFORM;
    platform->start_y = pos.y;

    platform->move = 0;
    platform->amplitude = 0;
    platform->offset = 0;

    return e;
}

Entity* spawn_timed_platforms_new(Vector2D pos, int platform_count){
    Entity* e = entity_new();
    e->type = ENT_INTERACTABLE;
    e->think = timed_platform_button_think;
    vector2d_copy(e->position, pos);
    e->sprite = gf2d_sprite_load_all("images/timer_button.png", 64, 32, 2);
    e->hurtbox = (Rect){{pos.x, pos.y},{e->sprite->frame_w, e->sprite->frame_h}};
    e->scale = vector2d(1,1);
    e->draw_offset = vector2d(0,0);

    TimePlatformButtonData* button = (TimePlatformButtonData*)e->data;
    button->type = TIMER_PLATFORM_BUTTON;
    button->activated = 0;
    button->time_max = 500;
    button->time_remaining = button->time_max;

    for (int i = 0; i < platform_count; i++){
        pos.x += 32;
        spawn_timed_platform_new(e, pos);
    }
    

    return e;   
}

Entity* spawn_falling_platform(Vector2D pos, int time_before_fall){
    Entity* e = entity_new();
    e->type = ENT_INTERACTABLE;
    e->think = falling_platform_think;

    vector2d_copy(e->position, pos);
    e->sprite = gf2d_sprite_load_image("images/drop_platform.png");
    e->hurtbox = (Rect){{pos.x, pos.y},{e->sprite->frame_w, e->sprite->frame_h}};
    e->scale = vector2d(1,1);
    e->draw_offset = vector2d(0,0);

    FallingPlatformData* d = (FallingPlatformData*)e->data;
    d->type = FALLING_PLATFORM;
    d->time_before_fall = time_before_fall;
    d->start_y = pos.y;
    d->should_fall = 0;

    return e;
}
