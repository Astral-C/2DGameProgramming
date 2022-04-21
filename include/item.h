#ifndef __ITEM_H__
#define __ITEM_H__
#include "pg_entity.h"
#include "inventory.h"

typedef struct {
    Uint8 count;
    Uint16 lifetime;
    ConsumableType type;
    char* item_name; // this will be used for special non consumable items
} PickupProps;

Entity* spawn_potion(ConsumableType type);
Entity* spawn_craft_item(CraftType type);

#endif