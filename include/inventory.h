#ifndef __INVENTORY_H__
#define __INVENTORY_H__
#include <SDL.h>
#include "gf2d_sprite.h"

typedef enum {
    HEALTH,
    SPEED,
    STAMINA,
    INVISIBILITY,
    TIME_FREEZE,
    CONSUMABLE_COUNT,
    NONE
} ConsumableType;

typedef struct ITEM_S {
    char* name;
    char* description;
} Item;

void inventory_init();
void inventory_cleanup();

void inventory_manager_save(char* path);
void inventory_manager_load(char* path);

void inventory_add_consumable(ConsumableType type, Uint8 count);
Uint8 inventory_use_consumable(ConsumableType type);

void inventory_register_item(char* name, char* description);
void inventory_unregister_item(char* name, char* description);

Sprite* inventory_manager_consumables_img();

void inventory_show_consumables();


#endif