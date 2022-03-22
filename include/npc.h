#ifndef __NPC_H__
#define __NPC_H__
#include "simple_json.h"
#include "pg_entity.h"
#include "pg_ui.h"

Entity* npc_new(char* img);
Entity* npc_shop_new(char* img, SJson* shop_data);
Uint8 npc_request_interaction();
Uint8 npc_is_interacting();

void draw_shop_ui();

#endif