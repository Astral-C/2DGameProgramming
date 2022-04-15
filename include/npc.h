#ifndef __NPC_H__
#define __NPC_H__
#include "simple_json.h"
#include "pg_entity.h"
#include "pg_ui.h"

typedef struct {
    Entity* interacting;
    Uint8 requested_interaction;
    Uint8 current_page;
} InteractionManager;


typedef struct {
    SJson* npc_json; // 5
    Sprite* textbox_text; // 9
    Uint32 textbox_pages; // 12
    Uint32 sell_type; // 16
    Uint32 sell_price; // 20
    Uint8 is_shop; // 1
    Uint8 has_quest;
} NpcProps;

Entity* npc_new(char* img);
Entity* npc_shop_new(char* img, SJson* shop_data);
Uint8 npc_request_interaction();
Uint8 npc_is_interacting();

void draw_shop_ui();

#endif