#include "enemy.h"
#include "simple_logger.h"
#include "simple_json.h"
#include "map.h"
#include "item.h"
#include "player.h"
#include "quest.h"

static struct {
    struct {
        Uint8 health;
        Uint8 range;
        Uint16 move_timer;
    } BatConfig;
    
    struct {
        Uint8 health;
        Uint8 amplitude;
        Uint16 lifetime;
        float spawn_y;
        float start;
    } SkullConfig;
    
    struct {
        Uint8 health;
        Uint16 range; 
        Uint8 charge_speed;
        Uint8 charge_timer;
    } GolemConfig;
    
    struct {
        Uint8 health;
        Uint16 range;
        Uint8 attack_timer;
    } MushConfig;
    
    struct {
        Uint8 health;
        Uint16 throw_timer;
        Uint16 throw_range;
        Uint16 wander_range;
        Uint16 idle_timer;
    } MagicianConfig;
} EnemyConfigs = {0};

/*

    Enemy think Functions

*/

void enemy_spawner_think(Entity* self){
    EnemySpawnerProps* spawner = (EnemySpawnerProps*)self->data;
    
    spawner->_time_next_spawn--;

    if(spawner->_time_next_spawn == 0 &&  spawner->_remaining_ents < spawner->max_entities){
        Entity* ent = NULL;
        switch (spawner->spawn_type){
        case ENEMY_BAT:
            ent = bat_new();
            ent->owner = self;
            spawner->_remaining_ents++;
            break;

        case ENEMY_SKULL:
            ent = skull_new();
            ent->owner = self;
            spawner->_remaining_ents++;
            ((SkullEnemyProps*)ent->data)->spawn_y = self->position.y;
            ent->velocity.x = 3;
            break;

        default:
            break;
        }

        ent->position = self->position;
        spawner->_time_next_spawn = spawner->spawn_interval;
    }
}

void bat_think(Entity* self){
    BatEnemyProps* bat = (BatEnemyProps*)self->data;
    bat->move_timer--;
    
    Entity* player = entity_manager_get_player();

    switch (bat->state){
    case HOME:
        if(bat->move_timer == 0){
            vector2d_copy(bat->home, self->position);
            self->velocity = vector2d(0,0);
            bat->state = WANDER;
        }
        break;

    case IDLE:
        if(bat->move_timer == 0){
            bat->state = WANDER;
        }
        break;

    case WANDER:
        if(abs(self->position.x - bat->home.x) > 100 || abs(self->position.y - bat->home.y) > 100){
            vector2d_sub(self->velocity, bat->home, self->position);
            vector2d_set_magnitude(&self->velocity, 3 * gfc_random());
        } else if(self->position.x == bat->home.x && self->position.y == bat->home.y){
            self->velocity = vector2d(gfc_crandom(), gfc_crandom());
            vector2d_set_magnitude(&self->velocity, 3 * gfc_random());
        }

        if(bat->move_timer == 0){
            bat->state = IDLE;
        }

        break;
    
    default:
        break;
    }
    
    if(rect_collider(self->hurtbox, player->hurtbox)){
        player->velocity.y -= 0.5;
        player->velocity.x = self->velocity.x * 2.5;
        player->health -= 1;
    }


    if(bat->move_timer == 0) bat->move_timer = EnemyConfigs.BatConfig.move_timer; //600

    //play flying animation
    self->frame += 0.01;
    if(self->frame > 2){
        self->frame = 0;
    }

    if(self->health <= 0){
        if(self->owner) ((EnemySpawnerProps*)self->owner->data)->_remaining_ents--;
        
        if(gfc_random() < 0.4){
            Entity* potion = spawn_potion(HEALTH);
            vector2d_copy(potion->position, self->position);
        }

        quest_manager_notify(ET_KillEnemy, ENEMY_BAT);

        ui_manager_free(bat->healthbar);
        entity_free(self);
    }
}

void golem_think(Entity* self){
    GolemEnemyProps* golem = (GolemEnemyProps*)self->data;
    Entity* player = entity_manager_get_player();
    float player_dist_x = fabs(self->position.x - player->position.x);
    float player_dist_y = fabs(self->position.y - player->position.y);
    switch (golem->state){
    case IDLE:
        if(player_dist_x < golem->range && player_dist_y < golem->range && player->visible){
            golem->state = WANDER;
            self->frame = 4;
        }
        break;

    case WANDER:
        if(!(player_dist_x < golem->range && player_dist_y < golem->range) || !player->visible){
            golem->state = IDLE;
            self->frame = 0;
            golem->charge_timer = EnemyConfigs.GolemConfig.charge_timer;
        }
        if(golem->charge_timer > 0){
            golem->charge_timer--;
        } else {
            if((self->position.x - player->position.x) > 0){
                self->velocity.x = -golem->charge_speed;
            } else {
                self->velocity.x = golem->charge_speed;
            }
            self->frame = 1;
            golem->charge_timer = EnemyConfigs.GolemConfig.charge_timer;
            golem->state = ATTACK;
        }

        break;
    case ATTACK:
        if(self->velocity.x == 0){
            golem->state = IDLE;
            self->frame = 0;
        }

        if(golem->charge_timer > 0){
            golem->charge_timer--;
        } else {
            if(!(player_dist_x < golem->range && player_dist_y < golem->range) || !player->visible){
                golem->state = IDLE;
                self->velocity.x = 0;
                self->frame = 0;
            }
            golem->charge_timer = EnemyConfigs.GolemConfig.charge_timer;
            
        }

        if(rect_collider(self->hurtbox, player->hurtbox)){
            player->velocity.y -= 8;
            player->velocity.x = self->velocity.x * 3;
            player->health -= 5;
            golem->state = IDLE;
            self->frame = 0;
            golem->charge_timer = EnemyConfigs.GolemConfig.charge_timer;
            self->velocity.x = 0;
        }
    
    default:
        break;
    }

    if(!(ent_collide_world(self) & COL_FLOOR)){
        self->velocity.y = 4;
    }

    if(self->health <= 0){
        float r = gfc_random();
        if(r < 0.35){
            Entity* potion = spawn_potion(SPEED);
            vector2d_copy(potion->position, self->position);
        }
        if(r < 0.9){
            player_add_money(100);
        }

        quest_manager_notify(ET_KillEnemy, ENEMY_GOLEM);

        ui_manager_free(golem->healthbar);
        entity_free(self);
    }

}

void skull_think(Entity* self){
    SkullEnemyProps* skull = (SkullEnemyProps*)self->data;

    Entity* player = entity_manager_get_player();

    skull->start += 0.1;
    self->position.y = skull->spawn_y + skull->amplitude * sin(skull->start);
    skull->lifetime--;

    if(self->health <= 0 || skull->lifetime == 0){
        if(self->owner) ((EnemySpawnerProps*)self->owner->data)->_remaining_ents--;
        
        if(self->health <= 0){
            if(gfc_random() < 0.4){
                Entity* potion = spawn_potion(INVISIBILITY);
                vector2d_copy(potion->position, self->position);
            }
        }

        ui_manager_free(skull->healthbar);
        entity_free(self);
        return;
    }

    if(rect_collider(self->hurtbox, player->hurtbox)){
        player->health -= 8;
        ui_manager_free(skull->healthbar);
        entity_free(self);
        return;
    }
}

void throw_atk_think(Entity* self){
    Entity* player = entity_manager_get_player();

    self->velocity.y = (self->velocity.y > 2 ? 2 : self->velocity.y + 0.2);

    if(ent_collide_world(self)){
        entity_free(self);
        return; 
    }

    if(rect_collider(player->hurtbox, self->hurtbox)){
        player->health -= 5;
        entity_free(self);
    }
}

void mushroom_think(Entity* self){
    MushroomEnemyProps* props = (MushroomEnemyProps*)self->data;
    Entity* player = entity_manager_get_player();

    if(props->attack_timer != 0){
        props->attack_timer--;
    }

    if(props->attack_timer == 0 && abs(player->position.x - self->position.x) < props->range && abs(player->position.y - self->position.y) < props->range && player->visible){
        Entity* ent = throwable_atk_new(self, "images/mush_atk.png");
        ent->velocity.x = (self->position.x - player->position.x > 0 ?  -4 : 4);
        ent->velocity.y = -4;
        props->attack_timer = EnemyConfigs.MushConfig.attack_timer;
    }

    if(self->health <= 0){
        if(gfc_random() < 0.4){
            Entity* potion = spawn_potion(TIME_FREEZE);
            vector2d_copy(potion->position, self->position);
        }        
        entity_free(self);
    }
}

void magician_think(Entity* self){
    MagicianEnemyProps* magician = (MagicianEnemyProps*)self->data;
    Entity* player = entity_manager_get_player();

    if(!magician->home_set){
        magician->home = self->position;
        magician->home_set = 1;
    }

    float player_dist_x = fabs(self->position.x - player->position.x);
    float player_dist_y = fabs(self->position.y - player->position.y);
    switch (magician->state){
    case IDLE:
        magician->idle_timer--;
        if(magician->idle_timer == 0){
            if (fabs(self->position.x - magician->home.x) >= magician->wander_range){
                if(self->position.x - magician->home.x >= 0){
                    self->velocity.x = -2;
                    self->flip = vector2d(0, 0);
                } else {
                    self->velocity.x = 2;
                    self->flip = vector2d(-1, 0);
                }
            } else {
                self->velocity.x = (gfc_random() > 0.5 ? -2 : 2);
                if(self->velocity.x < 0 && self->flip.x != 0){
                    self->flip = vector2d(0, 0);
                } else {
                    self->flip = vector2d(-1,0);
                }

            }
            
            magician->idle_timer = EnemyConfigs.MagicianConfig.idle_timer;
            self->frame = 1;
            magician->state = WANDER;
        }

        if(player_dist_x < magician->throw_range && player_dist_y < magician->throw_range && player->visible){
            self->frame = 3;
            magician->state = ATTACK;
        }
        break;

    case WANDER:
        magician->idle_timer--;
        self->frame += 0.1;
        if(self->frame > 3){
            self->frame = 1;
        }

        if(fabs(self->position.x - magician->home.x) > magician->wander_range || magician->idle_timer == 0){
            magician->idle_timer = EnemyConfigs.MagicianConfig.idle_timer;
            self->velocity.x = 0;
            self->frame = 0;
            magician->state = IDLE;
        }

        if((player_dist_x < magician->throw_range) && (player_dist_y < magician->throw_range) && player->visible){
            magician->state = ATTACK;
            self->frame = 3;
            self->velocity.x = 0;
        }
        
        break;

    case ATTACK:
        magician->throw_timer--;
        if(magician->throw_timer <= 0){
            Entity* ent = throwable_atk_new(self, "images/magician_atk.png");
            ent->velocity.x = (self->position.x - player->position.x > 0 ?  -6 : 6);
            ent->velocity.y = -2.5;
            magician->throw_timer = EnemyConfigs.MagicianConfig.throw_timer;
        }

        if(self->position.x - player->position.x > 0){
            self->flip = vector2d(0, 0);
        } else {
            self->flip = vector2d(1, 0);
        }

        if((player_dist_x > magician->throw_range && player_dist_y > magician->throw_range) || !player->visible){
            magician->state = WANDER;
        }
        break;

    default:
        break;
    }

    if(!(ent_collide_world(self) & COL_FLOOR)){
        self->velocity.y = 4;
    }

    if(self->health <= 0){
        float r = gfc_random();
        if(r < 0.4){
            Entity* potion = spawn_potion(STAMINA);
            vector2d_copy(potion->position, self->position);
        }
        if(r < 0.7){
            player_add_money(8);
        }
        ui_manager_free(magician->healthbar);
        entity_free(self);
    }
}

/*

    Enemy Spawn Functions

*/

Entity* enemy_spawner_new(EnemyType type, Uint8 spawn_count, Uint8 spawn_interval, Uint8 max_entities){
    Entity* ent = entity_new();
    EnemySpawnerProps* spawner = (EnemySpawnerProps*)ent->data;

    if(!ent){
        slog("no entities available");
    }

    spawner->spawn_type = type;
    spawner->spawn_amount = spawn_count;
    spawner->max_entities = max_entities;
    spawner->spawn_interval = spawn_interval * 600;
    spawner->_time_next_spawn = spawner->spawn_interval;

    ent->think = enemy_spawner_think;

    return ent;
}

Entity* bat_new(){
    Entity* ent = entity_new();
    BatEnemyProps* bat = (BatEnemyProps*)ent->data;

    bat->state = HOME;
    bat->healthbar = ui_manager_add_health(ent, EnemyConfigs.BatConfig.health);

    ent->sprite = gf2d_sprite_load_all("images/bat.png", 16, 16, 2);
    ent->health = EnemyConfigs.BatConfig.health;
    ent->think = bat_think;
    ent->scale = vector2d(4,4);
    ent->velocity = vector2d(gfc_crandom(), gfc_crandom()); // choose a random direction to move in to find a home
    bat->move_timer = EnemyConfigs.BatConfig.move_timer;
    ent->hurtbox = (Rect){{ent->position.x, ent->position.y}, {16*4, 16*4}};
    ent->type = ENT_ENEMY;
    
    return ent;
}

Entity* golem_new(){
    Entity* ent = entity_new();
    GolemEnemyProps* golem = (GolemEnemyProps*)ent->data;

    golem->state = IDLE;
    golem->healthbar = ui_manager_add_health(ent, EnemyConfigs.GolemConfig.health);
    golem->charge_speed = EnemyConfigs.GolemConfig.charge_speed;
    golem->charge_timer = EnemyConfigs.GolemConfig.charge_timer;
    golem->range = EnemyConfigs.GolemConfig.range;
    ent->health = EnemyConfigs.GolemConfig.health;

    ent->sprite = gf2d_sprite_load_all("images/golem.png", 128, 128, 7);
    ent->scale = vector2d(1,1);
    ent->draw_offset = vector2d(-28, -64);
    ent->think = golem_think;
    ent->hurtbox = (Rect){{ent->position.x, ent->position.y}, {64, 64}};
    ent->frame = 0;
    ent->type = ENT_ENEMY;

    return ent;
}

Entity* skull_new(){
    Entity* ent = entity_new();
    SkullEnemyProps* skull = (SkullEnemyProps*)ent->data;

    skull->state = IDLE;
    ent->health = EnemyConfigs.SkullConfig.health;
    skull->healthbar = ui_manager_add_health(ent, EnemyConfigs.SkullConfig.health);
    skull->start = 0;
    skull->amplitude = EnemyConfigs.SkullConfig.amplitude;
    skull->lifetime = EnemyConfigs.SkullConfig.lifetime;
    
    ent->sprite = gf2d_sprite_load_all("images/skull.png", 64, 64, 2);
    ent->scale = vector2d(1,1);
    ent->draw_offset = vector2d(0, 0);
    ent->think = skull_think;
    ent->flip = vector2d(-1,0);
    ent->hurtbox = (Rect){{ent->position.x, ent->position.y}, {64, 64}};
    ent->frame = 0;
    ent->type = ENT_ENEMY;
    
    return ent; 
}

Entity* mushroom_new(){
    Entity* ent = entity_new();
    MushroomEnemyProps* mushroom = (MushroomEnemyProps*)ent->data;

    mushroom->state = IDLE;
    ent->health = 5;
    mushroom->attack_timer = EnemyConfigs.MushConfig.attack_timer;
    mushroom->range = EnemyConfigs.MushConfig.range;
    
    ent->sprite = gf2d_sprite_load_all("images/mush.png", 32, 32, 1);
    ent->scale = vector2d(1,1);
    ent->draw_offset = vector2d(0, 0);
    ent->think = mushroom_think;
    ent->hurtbox = (Rect){{ent->position.x, ent->position.y}, {32, 32}};
    ent->frame = 0;
    ent->type = ENT_ENEMY;

    return ent; 
}

Entity* throwable_atk_new(Entity* ent, char* sprite_path){
    Entity* atk = entity_new();

    atk->sprite = gf2d_sprite_load_all(sprite_path, 32, 32, 1);
    atk->scale = vector2d(1,1);
    atk->draw_offset = vector2d(0, 0);
    atk->think = throw_atk_think;
    vector2d_sub(atk->position, ent->position, vector2d(0, -2));

    atk->owner = ent;

    atk->hurtbox = (Rect){{ent->position.x, ent->position.y}, {32, 32}};
    atk->frame = 0;

    return atk;
}

Entity* magician_new(){
    Entity* ent = entity_new();
    MagicianEnemyProps* magician = (MagicianEnemyProps*)ent->data;

    magician->state = IDLE;
    magician->healthbar = ui_manager_add_health(ent, EnemyConfigs.MagicianConfig.health);
    ent->health = EnemyConfigs.MagicianConfig.health;
    magician->throw_timer = EnemyConfigs.MagicianConfig.throw_timer;
    magician->throw_range = EnemyConfigs.MagicianConfig.throw_range;
    magician->wander_range = EnemyConfigs.MagicianConfig.wander_range;
    magician->idle_timer = EnemyConfigs.MagicianConfig.idle_timer;
    magician->home_set = 0;
    
    ent->sprite = gf2d_sprite_load_all("images/magician.png", 64, 64, 4);
    ent->scale = vector2d(1,1);
    ent->draw_offset = vector2d(0, 0);
    ent->think = magician_think;
    ent->hurtbox = (Rect){{ent->position.x, ent->position.y}, {64, 64}};
    ent->frame = 0;
    ent->type = ENT_ENEMY;
    
    return ent; 
}


void load_enemy_configs(){
    SJson *bat_def, *golem_def, *skull_def, *mush_def, *mag_def;
    bat_def = sj_load("bat_def.json");
    golem_def = sj_load("golem_def.json");
    skull_def = sj_load("skull_def.json");
    mush_def = sj_load("mush_def.json");
    mag_def = sj_load("magician_def.json");

    if(!bat_def){
        EnemyConfigs.BatConfig.health = 10;
        EnemyConfigs.BatConfig.move_timer = 600;
    } else {
        SJson *health, *move_timer;
        health = sj_object_get_value(bat_def, "health");
        move_timer = sj_object_get_value(bat_def, "move_timer");
        
        int hp, mt;
        if(!sj_get_integer_value(health, &hp)){
            EnemyConfigs.BatConfig.health = 10;
        } else {
            EnemyConfigs.BatConfig.health = hp;
        }

        if(!sj_get_integer_value(move_timer, &mt)){
            EnemyConfigs.BatConfig.move_timer = 600;
        } else {
            EnemyConfigs.BatConfig.move_timer = mt;
        }

        sj_free(bat_def);
    }

    if(!golem_def){
        EnemyConfigs.GolemConfig.health = 200;
        EnemyConfigs.GolemConfig.charge_speed = 3;
        EnemyConfigs.GolemConfig.charge_timer = 100;
        EnemyConfigs.GolemConfig.range = 400;
    } else {
        SJson *health, *charge_speed, *charge_timer, *range;
        health = sj_object_get_value(golem_def, "health");
        charge_speed = sj_object_get_value(golem_def, "charge_speed");
        charge_timer = sj_object_get_value(golem_def, "charge_timer");
        range = sj_object_get_value(golem_def, "range");

        int hp, cs, ct, rng;
        if(!sj_get_integer_value(health, &hp)){
            EnemyConfigs.GolemConfig.health = 10;
        } else {
            EnemyConfigs.GolemConfig.health = hp;
        }

        if(!sj_get_integer_value(charge_speed, &cs)){
            EnemyConfigs.GolemConfig.charge_speed = 3;
        } else {
            EnemyConfigs.GolemConfig.charge_speed = cs;
        }

        if(!sj_get_integer_value(charge_timer, &ct)){
            EnemyConfigs.GolemConfig.charge_timer = 100;
        } else {
            EnemyConfigs.GolemConfig.charge_timer = ct;
        }

        if(!sj_get_integer_value(range, &rng)){
            EnemyConfigs.GolemConfig.range = 400;
        } else {
            EnemyConfigs.GolemConfig.range = rng;
        }

        sj_free(golem_def);
    }

    if(!skull_def){
        EnemyConfigs.SkullConfig.health = 50;
        EnemyConfigs.SkullConfig.amplitude = 50;
        EnemyConfigs.SkullConfig.lifetime = 1000;
    } else {
        SJson *health, *amplitude, *lifetime;
        health = sj_object_get_value(skull_def, "health");
        amplitude = sj_object_get_value(skull_def, "amplitude");
        lifetime = sj_object_get_value(skull_def, "lifetime");
        
        int hp, amp, lt;
        if(!sj_get_integer_value(health, &hp)){
            EnemyConfigs.SkullConfig.health = 50;
        } else {
            EnemyConfigs.SkullConfig.health = hp;
        }

        if(!sj_get_integer_value(amplitude, &amp)){
            EnemyConfigs.SkullConfig.amplitude = 50;
        } else {
            EnemyConfigs.SkullConfig.amplitude = amp;
        }

        if(!sj_get_integer_value(lifetime, &lt)){
            EnemyConfigs.SkullConfig.lifetime = 1000;
        } else {
            EnemyConfigs.SkullConfig.lifetime = lt;
        }

        sj_free(skull_def);
    }

    if(!mush_def){
        EnemyConfigs.MushConfig.health = 5;
        EnemyConfigs.MushConfig.attack_timer = 120;
        EnemyConfigs.MushConfig.range = 350;
    } else {
        SJson *health, *attack_timer, *range;
        health = sj_object_get_value(mush_def, "health");
        attack_timer = sj_object_get_value(mush_def, "attack_timer");
        range = sj_object_get_value(mush_def, "range");
        
        int hp, at, rn;
        if(!sj_get_integer_value(health, &hp)){
            EnemyConfigs.MushConfig.health = 5;
        } else {
            EnemyConfigs.MushConfig.health = hp;
        }

        if(!sj_get_integer_value(attack_timer, &at)){
            EnemyConfigs.MushConfig.attack_timer = 120;
        } else {
            EnemyConfigs.MushConfig.attack_timer = at;
        }

        if(!sj_get_integer_value(range, &rn)){
            EnemyConfigs.MushConfig.range = 350;
        } else {
            EnemyConfigs.MushConfig.range = rn;
        }

        sj_free(mush_def);
    }

    if(!mag_def){
        EnemyConfigs.MagicianConfig.health = 25;
        EnemyConfigs.MagicianConfig.idle_timer = 150;
        EnemyConfigs.MagicianConfig.throw_range = 50;
        EnemyConfigs.MagicianConfig.throw_timer = 60;
        EnemyConfigs.MagicianConfig.wander_range = 450;
    } else {
        SJson *health, *idle_timer, *throw_range, *throw_timer, *wander_range;
        health = sj_object_get_value(mag_def, "health");
        idle_timer = sj_object_get_value(mag_def, "idle_timer");
        throw_range = sj_object_get_value(mag_def, "throw_range");
        throw_timer = sj_object_get_value(mag_def, "throw_timer");
        wander_range = sj_object_get_value(mag_def, "wander_range");
        
        int hp, it, tr, tt, wr;
        if(!sj_get_integer_value(health, &hp)){
            EnemyConfigs.MagicianConfig.health = 25;
        } else {
            EnemyConfigs.MagicianConfig.health = hp;
        }

        if(!sj_get_integer_value(idle_timer, &it)){
            EnemyConfigs.MagicianConfig.idle_timer = 150;
        } else {
            EnemyConfigs.MagicianConfig.idle_timer = it;
        }

        if(!sj_get_integer_value(throw_range, &tr)){
            EnemyConfigs.MagicianConfig.throw_range = 50;
        } else {
            EnemyConfigs.MagicianConfig.throw_range = tr;
        }

        if(!sj_get_integer_value(throw_timer, &tt)){
            EnemyConfigs.MagicianConfig.throw_timer = 60;
        } else {
            EnemyConfigs.MagicianConfig.throw_timer = tt;
        }

        if(!sj_get_integer_value(wander_range, &wr)){
            EnemyConfigs.MagicianConfig.wander_range = 450;
        } else {
            EnemyConfigs.MagicianConfig.wander_range = wr;   
        }

        sj_free(mag_def);
    }

}