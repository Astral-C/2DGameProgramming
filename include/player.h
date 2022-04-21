#ifndef __PLAYER_H__
#define __PLAYER_H__
#include "pg_entity.h"

Entity* player_new();

void player_add_money(int amount);
Uint8 player_spend_money(int amount);
void player_draw_wallet();

void player_load(Entity* player, Vector2D position, Uint8 health, float stamina, int money);
void player_save(SJson* save);

#endif