#include "enemy.h"
#include "simple_logger.h"
#include "simple_json.h"
#include "map.h"
#include "item.h"

struct {
    SJson* bat;
    SJson* skull;
    SJson* golem;
    SJson* mushroom;
    SJson* magician;
} EnemyConfigs;

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
        }

        ent->position = self->position;
        spawner->_time_next_spawn = spawner->spawn_interval;
    }
}

void bat_think(Entity* self){
    BatEnemyProps* bat = (BatEnemyProps*)self->data;
    bat->move_timer--;
    
    Entity* player = entity_manager_get_player();

    //add player range check here

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
            vector2d_set_magnitude(&self->velocity, rand() % 3);
        } else if(self->position.x == bat->home.x && self->position.y == bat->home.y){
            self->velocity = vector2d(rand() % 2, rand() % 2);
        }

        if(bat->move_timer == 0){
            bat->state = IDLE;
        }

        break;
    }
    
        if(rect_collider(self->hurtbox, player->hurtbox)){
            player->velocity.y -= 2;
            player->velocity.x = self->velocity.x * 1.5;
            player->health -= 10;
        }


    if(bat->move_timer == 0) bat->move_timer = 600;

    //play flying animation
    self->frame += 0.01;
    if(self->frame > 2){
        self->frame = 0;
    }

    if(self->health <= 0){
        if(self->owner) ((EnemySpawnerProps*)self->owner->data)->_remaining_ents--;
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
        if(player_dist_x < golem->range && player_dist_y < golem->range){
            golem->state = WANDER;
            self->frame = 4;
        }
        break;

    case WANDER:
        if(!(player_dist_x < golem->range && player_dist_y < golem->range)){
            golem->state = IDLE;
            self->frame = 0;
            golem->charge_timer = 80;
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
            golem->charge_timer = 80;
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
            if(!(player_dist_x < golem->range && player_dist_y < golem->range)){
                golem->state = IDLE;
                self->velocity.x = 0;
                self->frame = 0;
            }
            golem->charge_timer = 80;
            
        }

        if(rect_collider(self->hurtbox, player->hurtbox)){
            player->velocity.y -= 8;
            player->velocity.x = self->velocity.x * 3;
            player->health -= 5;
            golem->state = IDLE;
            self->frame = 0;
            golem->charge_timer = 80;
            self->velocity.x = 0;
        }
    }

    ent_collide_world(self);

    if(self->health <= 0){
        if(gfc_random() < 0.2){
            Entity* potion = spawn_potion(SPEED);
            vector2d_copy(potion->position, self->position);
        }
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

    if(props->attack_timer == 0 && abs(player->position.x - self->position.x) < props->range && abs(player->position.y - self->position.y) < props->range){
        Entity* ent = throwable_atk_new(self, "images/mush_atk.png");
        ent->velocity.x = (self->position.x - player->position.x > 0 ?  -4 : 4);
        ent->velocity.y = -4;
        props->attack_timer = 120;
    }

    if(self->health <= 0){
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
                } else {
                    self->velocity.x = 2;
                }
            } else {
                self->velocity.x = (gfc_random() > 0.5 ? -2 : 2);
            }
            
            magician->idle_timer = 150;
            self->frame = 1;
            magician->state = WANDER;
        }

        if(player_dist_x < magician->throw_range && player_dist_y < magician->throw_range){
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
            magician->idle_timer = 350;
            self->velocity.x = 0;
            self->frame = 0;
            magician->state = IDLE;
        }

        if(player_dist_x < magician->throw_range && player_dist_y < magician->throw_range){
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
            magician->throw_timer = 60;
        }

        if(self->position.x - player->position.x > 0){
            self->flip = vector2d(0, 0);
        } else {
            self->flip = vector2d(1, 0);
        }

        if(player_dist_x > magician->throw_range && player_dist_y > magician->throw_range){
            magician->state = WANDER;
        }
        break;
    }

    ent_collide_world(self);

    if(self->health <= 0){
        if(gfc_random() < 0.4){
            Entity* potion = spawn_potion(STAMINA);
            vector2d_copy(potion->position, self->position);
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
}

Entity* bat_new(){
    Entity* ent = entity_new();
    BatEnemyProps* bat = (BatEnemyProps*)ent->data;

    bat->state = HOME;
    bat->swoop_timer = 10; //remove this?
    bat->healthbar = ui_manager_add_health(ent, 20);

    ent->sprite = gf2d_sprite_load_all("images/bat.png", 16, 16, 2);
    ent->health = 20;
    ent->think = bat_think;
    ent->scale = vector2d(4,4);
    ent->velocity = vector2d(rand() % 2, rand() % 2); // choose a random direction to move in to find a home
    bat->move_timer = 600;
    ent->hurtbox = (Rect){ent->position.x, ent->position.y, 16*4, 16*4};
    
    return ent;
}

Entity* golem_new(){
    Entity* ent = entity_new();
    GolemEnemyProps* golem = (GolemEnemyProps*)ent->data;

    golem->state = IDLE;
    golem->healthbar = ui_manager_add_health(ent, 200);
    golem->charge_speed = 3;
    golem->charge_timer = 100;
    golem->range = 200;
    ent->health = 200;

    ent->sprite = gf2d_sprite_load_all("images/golem.png", 128, 128, 7);
    ent->scale = vector2d(1,1);
    ent->draw_offset = vector2d(-28, -64);
    ent->think = golem_think;
    ent->hurtbox = (Rect){ent->position.x, ent->position.y, 64, 64};
    ent->frame = 0;
    
    return ent;
}

Entity* skull_new(){
    Entity* ent = entity_new();
    SkullEnemyProps* skull = (SkullEnemyProps*)ent->data;

    skull->state = IDLE;
    ent->health = 50;
    skull->healthbar = ui_manager_add_health(ent, 50);
    skull->start = 0;
    skull->amplitude = 50;
    skull->lifetime = 1000;
    
    ent->sprite = gf2d_sprite_load_all("images/skull.png", 64, 64, 2);
    ent->scale = vector2d(1,1);
    ent->draw_offset = vector2d(0, 0);
    ent->think = skull_think;
    ent->flip = vector2d(-1,0);
    ent->hurtbox = (Rect){ent->position.x, ent->position.y, 64, 64};
    ent->frame = 0;
    
    return ent; 
}

Entity* mushroom_new(){
    Entity* ent = entity_new();
    MushroomEnemyProps* mushroom = (MushroomEnemyProps*)ent->data;

    mushroom->state = IDLE;
    ent->health = 5;
    mushroom->attack_timer = 120;
    mushroom->range = 350;
    
    ent->sprite = gf2d_sprite_load_all("images/mush.png", 32, 32, 1);
    ent->scale = vector2d(1,1);
    ent->draw_offset = vector2d(0, 0);
    ent->think = mushroom_think;
    ent->hurtbox = (Rect){ent->position.x, ent->position.y, 32, 32};
    ent->frame = 0;
    
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

    atk->hurtbox = (Rect){ent->position.x, ent->position.y, 32, 32};
    atk->frame = 0;

    return atk;
}

Entity* magician_new(){
    Entity* ent = entity_new();
    MagicianEnemyProps* magician = (MagicianEnemyProps*)ent->data;

    magician->state = IDLE;
    magician->healthbar = ui_manager_add_health(ent, 25);
    ent->health = 25;
    magician->throw_timer = 60;
    magician->throw_range = 100;
    magician->wander_range = 250;
    magician->idle_timer = 200;
    magician->home_set = 0;
    
    ent->sprite = gf2d_sprite_load_all("images/magician.png", 64, 64, 4);
    ent->scale = vector2d(1,1);
    ent->draw_offset = vector2d(0, 0);
    ent->think = magician_think;
    ent->hurtbox = (Rect){ent->position.x, ent->position.y, 64, 64};
    ent->frame = 0;
    
    return ent; 
}
