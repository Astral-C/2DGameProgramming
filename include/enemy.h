#ifndef __ENEMY_H__
#define __ENEMY_H__
#include "pg_entity.h"
#include "pg_ui.h"

//need two more 
typedef enum {
    ENEMY_BAT,
    ENEMY_SKULL,
    ENEMY_GOLEM,
    ENEMY_MUSHROOM,
    ENEMY_MAGICIAN
} EnemyType;

typedef enum {
    HOME,
    IDLE,
    WANDER,
    ATTACK
} EnemyState;

typedef struct {
    Uint8 _time_next_spawn;
    Uint8 _remaining_ents;
    Uint8 spawn_amount;
    Uint8 spawn_interval;
    Uint8 max_entities;
    EnemyType spawn_type;
} EnemySpawnerProps;

typedef struct {
    Uint16 move_timer;
    UIElement* healthbar;
    EnemyState state;
    Vector2D home;
} BatEnemyProps;

typedef struct {
    UIElement* healthbar;
    EnemyState state;
    Uint16 range;        // How close a player must stand to a golem's home 
    Uint8 charge_speed; // How fast a golem moves when it charges
    Uint8 charge_timer; // How long a player must stand in range of a golem before it charges
} GolemEnemyProps;

typedef struct {
    UIElement* healthbar;
    EnemyState state;
    Uint8 amplitude;
    float start;
    float spawn_y;
    Uint16 lifetime;
} SkullEnemyProps;

typedef struct {
    //UIElement* healthbar;
    EnemyState state;
    Uint8 attack_timer;
    Uint16 range;
} MushroomEnemyProps;

typedef struct {
    UIElement* healthbar;
    EnemyState state;
    Uint16 throw_timer;
    Uint16 throw_range;
    Uint16 wander_range;
    Uint16 idle_timer;
    Vector2D home;
    Uint8 home_set;
} MagicianEnemyProps;

Entity* enemy_spawner_new(EnemyType type, Uint8 spawn_count, Uint8 spawn_interval, Uint8 max_entities);

Entity* bat_new();
Entity* golem_new();
Entity* skull_new();

Entity* mushroom_new();
Entity* magician_new();

Entity* throwable_atk_new(Entity* thrower, char* sprite_path);

void load_enemy_configs();

#endif