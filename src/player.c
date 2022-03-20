#include "player.h"
#include "map.h"
#include "w_axe.h"
#include "w_bomb.h"
#include "w_knife.h"
#include "w_shuriken.h"
#include "simple_logger.h"
#include "pg_ui.h"
#include <math.h>

typedef enum {
    BOMB,
    KNIFE,
    AXE,
    SHUR,
    WEAPON_COUNT
} Weapon;

typedef enum {
    IDLE,
    WALK,
    FALLING,
    JUMP
} PlayerState;

typedef struct {
    PlayerState state;
    int weapon_cooldown;
    int direction;
    Weapon active_weapon;
    Uint8 on_floor;
    UIElement* healthbar;
} PlayerData;

void player_think(Entity *self){
    if(!self) return;
    PlayerData* pd = (PlayerData*)self->data;

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    switch (pd->state){
    case IDLE:
        self->frame = (self->frame + 0.03);
        if(self->frame > 2) self->frame = 0;

        if(keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_RIGHT]){
            pd->state = WALK;
        }

        if(keys[SDL_SCANCODE_UP]){
            self->velocity.y -= 10;
            pd->state = FALLING;
        }

        if(self->velocity.y != 0){
            pd->state = FALLING;
        }

        break;
    
    case WALK:
        self->frame = (self->frame + 0.03);
        if(self->frame > 4) self->frame = 2;

        if(!pd->on_floor){
            pd->state = FALLING;
        }

        if(!keys[SDL_SCANCODE_LSHIFT]){
            if((keys[SDL_SCANCODE_LEFT] && !pd->direction) || (keys[SDL_SCANCODE_RIGHT] && pd->direction)) pd->direction = !pd->direction;
        }

        if(keys[SDL_SCANCODE_LEFT]){
            self->velocity.x = -2;
        } else if (keys[SDL_SCANCODE_RIGHT]){
            self->velocity.x = 2;
        }
        
        if(self->velocity.x > 0){
            self->velocity.x = (self->velocity.x - 0.1 < 0.1 ? 0 : self->velocity.x - 0.1);
        } else if(self->velocity.x < 0){
            self->velocity.x = (self->velocity.x + 0.1 > -0.1 ? 0 : self->velocity.x + 0.1);
        } else if(self->velocity.x == 0){
            pd->state = IDLE;
        }

        if(self->velocity.y == 0 && keys[SDL_SCANCODE_UP]){
            self->velocity.y -= 10;
            pd->state = FALLING;
        }
        
        break;

    case FALLING:
        self->frame = 4;
        
        if(keys[SDL_SCANCODE_LEFT]){
            self->velocity.x = -2;
        } else if (keys[SDL_SCANCODE_RIGHT]){
            self->velocity.x = 2;
        }
        
        if(!keys[SDL_SCANCODE_LSHIFT]){
            if((keys[SDL_SCANCODE_LEFT] && !pd->direction) || (keys[SDL_SCANCODE_RIGHT] && pd->direction)) pd->direction = !pd->direction;
        }

        if(self->velocity.x > 0){
            self->velocity.x = (self->velocity.x - 0.005 < 0.005 ? 0 : self->velocity.x - 0.005);
        } else if(self->velocity.x < 0){
            self->velocity.x = (self->velocity.x + 0.005 > -0.005 ? 0 : self->velocity.x + 0.005);
        }

        if(pd->on_floor){
            if(self->velocity.x == 0){
                pd->state = IDLE;
            } else {
                pd->state = WALK;
            }
        } else {
            if(self->velocity.y < 6.0f) self->velocity.y += (keys[SDL_SCANCODE_UP] && self->velocity.y < 0 ? 0.2f : 0.6f) + (10.0f * keys[SDL_SCANCODE_DOWN]);
        }

        break;

    default:
        break;
    }

    self->flip = vector2d(pd->direction, 0);

    if(keys[SDL_SCANCODE_X] && pd->weapon_cooldown == 0){
        pd->active_weapon = (pd->active_weapon + 1) % WEAPON_COUNT;
        pd->weapon_cooldown = 20;
    }

    if(keys[SDL_SCANCODE_Z] && pd->weapon_cooldown == 0){
        int dir = (pd->direction ? -1 : 1);
        Entity* w = NULL;
        
        switch (pd->active_weapon)
        {
        case BOMB:
            w = bomb_ent_new();
            vector2d_add(w->velocity, w->velocity, vector2d(dir*3, -2));
            break;
        case KNIFE:
            w = knife_ent_new();
            w->flip = vector2d(pd->direction, 0);
            w->velocity.x *= dir;
            break;
        case AXE:
            w = axe_ent_new();
            w->flip = vector2d(pd->direction, 0);
            w->velocity.x *= dir;
            break;
        case SHUR:
            w = shuriken_ent_new();
            w->owner = self;
            w->flip = vector2d(pd->direction, 0);
            w->velocity.x *= dir;
            break;
        default:
            break;
        }

        if(w){
            w->owner = self;
            vector2d_add(w->position, self->position, vector2d(32, 32));
        }
        
        pd->weapon_cooldown = 50;
    }

    pd->on_floor = ent_collide_world(self) & COL_FLOOR;

    if(pd->weapon_cooldown > 0) pd->weapon_cooldown--;

}


Entity* player_new(){
    Entity* plyr = entity_new();
    PlayerData* pd = (PlayerData*)plyr->data;
    if(!plyr){
        //slog("no bugspace");
        return false;
    }
    pd->healthbar = ui_manager_add_health(plyr, 100);

    plyr->sprite = gf2d_sprite_load_all("images/plyr_sheet.png",16, 16, 18);
    plyr->think = player_think;
    
    plyr->scale = vector2d(4,4);
    pd->state = FALLING;
    plyr->health = 100;
    pd->active_weapon = KNIFE;
    pd->direction = 1;
    plyr->hurtbox = (Rect){plyr->position.x, plyr->position.y, 16*4, 16*4};

    pd->on_floor = 0;

    plyr->velocity.y = 1;
    return plyr;
}