#include "player.h"
#include "gfc_input.h"
#include "map.h"
#include "w_axe.h"
#include "w_bomb.h"
#include "w_knife.h"
#include "w_shuriken.h"
#include "simple_logger.h"
#include "inventory.h"
#include "pg_ui.h"
#include "npc.h"
#include "audio.h"
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
    JUMP,
    INTERACTING
} PlayerState;

typedef struct {
    Uint8 money_dirty;
    Uint32 money;
    Sprite* money_sprite;
} PlayerWallet;

typedef struct {
    Uint8 on_floor; // 1
    Uint16 effect_time; // 3
    ConsumableType active_effect; // 7

    PlayerState state; // 11
    int direction; // 15
    int fight_timer;
    int is_fighting;
    int whip_timer;
    float stamina; // 19
    Weapon active_weapon; // 23
} PlayerData;

static PlayerWallet wallet = {.money = 10, .money_dirty = 1, .money_sprite = NULL};

void player_add_money(int amount){
    wallet.money_dirty = 1;
    wallet.money += amount;
}

Uint8 player_spend_money(int amount){
    slog("wallet amount is %u, trying to spend %u", wallet.money, amount);
    if(amount <= wallet.money){
        wallet.money_dirty = 1;
        wallet.money -= amount;
        return 1;
    } else {
        return 0;
    }
}

void player_draw_wallet(){
    gf2d_sprite_draw_image(wallet.money_sprite, vector2d(1200 - wallet.money_sprite->frame_w, 0));
}

void player_think(Entity *self){
    if(!self) return;
    PlayerData* pd = (PlayerData*)self->data;

    Uint8 left_held, right_held, up_held, down_pressed, strafing, stomp;
    left_held = gfc_input_command_held("left");
    right_held = gfc_input_command_held("right");
    up_held = gfc_input_command_held("jump");
    down_pressed = gfc_input_command_released("stomp");
    stomp = gfc_input_command_pressed("stomp");
    strafing = gfc_input_command_held("strafe");

    if (down_pressed){
        try_warp(self->hurtbox);        
    }

    if(gfc_input_command_pressed("potion1") && inventory_use_consumable(HEALTH)){
        self->health = (self->health + 15 > 100 ? 100 : self->health + 15);
    } else if(gfc_input_command_pressed("potion2") && inventory_use_consumable(SPEED)){
        pd->active_effect = SPEED;
        pd->effect_time = 1000;
    } else if(gfc_input_command_pressed("potion3") && inventory_use_consumable(STAMINA)){
        pd->stamina = (pd->stamina + 25 > 100 ? 100 : pd->stamina + 25);
    } else if(gfc_input_command_pressed("potion4") && inventory_use_consumable(INVISIBILITY)){
        self->visible = 0;
        pd->effect_time = 1000;
    } else if(gfc_input_command_pressed("potion5") && inventory_use_consumable(TIME_FREEZE)){
        pd->active_effect = TIME_FREEZE;
        entity_manager_time_freeze();
        pd->effect_time = 1000;
    }

    if(pd->effect_time > 0){
        pd->effect_time--;
    } else if (pd->effect_time == 0){
        if(pd->active_effect == TIME_FREEZE) entity_manager_time_unfreeze();
        if(!self->visible) self->visible = 1;
        pd->active_effect = NONE;
    }

    switch (pd->state){
    case IDLE:
        self->frame = (self->frame + 0.03);
        if(self->frame > 2) self->frame = 0;

        if(left_held || right_held){
            pd->state = WALK;
        }

        if(up_held){
            self->velocity.y -= 10;
            pd->state = FALLING;
        }

        if(self->velocity.y != 0){
            pd->state = FALLING;
        }

        if(gfc_input_command_released("advance_text") && pd->on_floor && self->velocity.x == 0 && self->velocity.y == 0){
            npc_request_interaction();
            pd->state = INTERACTING;
        }

        break;
    
    case WALK:
        self->frame = (self->frame + 0.03);
        if(self->frame > 4) self->frame = 2;

        if(!pd->on_floor){
            pd->state = FALLING;
        }

        if(!strafing){
            if((left_held && !pd->direction) || (right_held && pd->direction)) pd->direction = !pd->direction;
        }

        if(left_held){
            self->velocity.x = (pd->active_effect == SPEED ? -6 : -4);
        } else if (right_held){
            self->velocity.x = (pd->active_effect == SPEED ? 6 : 4);
        }
        
        if(self->velocity.x > 0){
            self->velocity.x = (self->velocity.x - 0.5 < 0.6 ? 0 : self->velocity.x - 0.5);
        } else if(self->velocity.x < 0){
            self->velocity.x = (self->velocity.x + 0.5 > -0.6 ? 0 : self->velocity.x + 0.5);
        } else if(self->velocity.x == 0){
            pd->state = IDLE;
        }

        if(self->velocity.y == 0 && up_held){
            self->velocity.y -= 10;
            pd->state = FALLING;
        }

        break;

    case FALLING:
        self->frame = 4;
        
        if(left_held){
            self->velocity.x = (pd->active_effect == SPEED ? -6 : -4);
        } else if (right_held){
            self->velocity.x = (pd->active_effect == SPEED ? 6 : 4);
        }
        
        if(!strafing){
            if((left_held && !pd->direction) || (right_held && pd->direction)) pd->direction = !pd->direction;
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
            if(self->velocity.y < 6.0f) self->velocity.y += (up_held && self->velocity.y < 0 ? 0.2f : 0.6f) + (10.0f * stomp);
        }

        break;

    case INTERACTING:
        if(!npc_is_interacting()){
            pd->state = IDLE;
        }
        break;

    default:
        break;
    }

    self->flip = vector2d(pd->direction, 0);

    if(pd->state != INTERACTING){
        if(gfc_input_command_released("switch_weapon")){
            pd->active_weapon = (pd->active_weapon + 1) % WEAPON_COUNT;
        }

        if(gfc_input_command_released("fire_weapon") && pd->stamina > 0){
            int dir = (pd->direction ? -1 : 1);
            Entity* w = NULL;
            pd->fight_timer -= 25;
            switch (pd->active_weapon)
            {
            case BOMB:
                w = bomb_ent_new();
                vector2d_add(w->velocity, w->velocity, vector2d(dir*3, -2));
                pd->stamina -= 4;
                break;
            case KNIFE:
                w = knife_ent_new();
                w->flip = vector2d(pd->direction, 0);
                w->velocity.x *= dir;
                pd->stamina -= 2;
                break;
            case AXE:
                w = axe_ent_new();
                w->flip = vector2d(pd->direction, 0);
                w->velocity.x *= dir;
                pd->stamina -= 15;
                break;
            case SHUR:
                w = shuriken_ent_new();
                w->owner = self;
                w->flip = vector2d(pd->direction, 0);
                w->velocity.x *= dir;
                pd->stamina -= 5;
                break;
            default:
                break;
            }

            if(w){
                w->owner = self;
                vector2d_add(w->position, self->position, vector2d(32, 32));
            }

        }

        if(gfc_input_command_released("whip")){
            pd->whip_timer = 100;
        }

        if(pd->whip_timer > 0){
            self->frame = 5;
            pd->whip_timer -= 10;
            Rect whip_hb = self->hurtbox;
            whip_hb.pos.x += (32 * (pd->direction ? -1 : 1));
            whip_hb.pos.y += 32;
            whip_hb.size.y = 16;
            Entity* other = get_colliding_r(whip_hb, self);
            if(other){
                other->health -= 10;
            }
        }
    }

    pd->on_floor = ent_collide_world(self) & COL_FLOOR;

    if(wallet.money_dirty){
        slog("money dirty, updating...");
        char money_str[10];
        snprintf(money_str, sizeof(money_str), "$%d", wallet.money);
        if(wallet.money_sprite) gf2d_sprite_free(wallet.money_sprite);
        wallet.money_sprite = ui_manager_render_text(money_str, (SDL_Color){255,255,255,255});
        wallet.money_dirty = 0;
    }

    if(pd->stamina < 100) pd->stamina += 0.05;

    if(self->health <= 0){
        wallet.money = 0;
        wallet.money_dirty = 1;
        self->position = map_get_player_spawn();
        self->health = 100;
        inventory_clear();
    }

    if(pd->fight_timer < 0 && pd->is_fighting == 0){
        audio_open_mod("audio/musix-shine.mod");
        audio_play_mod();
        pd->is_fighting = 1;
    } else if(pd->fight_timer < 200){
        pd->fight_timer += 1;
    } else if(pd->is_fighting && pd->fight_timer == 200){
        pd->is_fighting = 0;
        map_manager_play_bgm();
    }

}

Entity* player_new(){
    Entity* plyr = entity_new();
    PlayerData* pd = (PlayerData*)plyr->data;
    if(!plyr){
        //slog("no bugspace");
        return false;
    }

    ui_manager_add_progressi(vector2d(105, 20), vector4d(0x50, 0x80, 0x60, 0xFF), 100, &plyr->health);
    ui_manager_add_progressf(vector2d(105, 55), vector4d(0x30, 0x80, 0xc0, 0xFF), 100, &pd->stamina);

    plyr->type = ENT_PLAYER;

    plyr->sprite = gf2d_sprite_load_all("images/plyr_sheet.png",16, 16, 18);
    plyr->think = player_think;
    
    plyr->scale = vector2d(4,4);
    pd->state = FALLING;
    plyr->health = 100;
    pd->active_weapon = KNIFE;
    pd->direction = 1;
    pd->stamina = 100;
    pd->fight_timer = 200;
    pd->is_fighting = 0;
    plyr->hurtbox = (Rect){{plyr->position.x, plyr->position.y}, {64, 64}};

    pd->on_floor = 0;

    plyr->velocity.y = 1;
    return plyr;
}

void player_load(Entity* player, Vector2D position, Uint8 health, float stamina, int money){
    wallet.money = money;
    wallet.money_dirty = 1;

    player->health = health;
    player->position = position;
    PlayerData* data = (PlayerData*)player->data;
    data->stamina = stamina;
}

void player_save(SJson* save){
    SJson* player_pos = sj_array_new();

    Entity* player_ent = entity_manager_get_player();
    PlayerData* data = (PlayerData*)player_ent->data;
    sj_object_insert(save, "health", sj_new_int(player_ent->health));
    sj_object_insert(save, "stamina", sj_new_float(data->stamina));
    sj_object_insert(save, "money", sj_new_int(wallet.money));

    sj_array_append(player_pos, sj_new_float(player_ent->position.x));
    sj_array_append(player_pos, sj_new_float(player_ent->position.y));
    sj_object_insert(save, "position", player_pos);
}