#ifndef __INVENTORY_H__
#define __INVENTORY_H__
#include <SDL.h>
#include "gf2d_sprite.h"
#include "simple_json.h"

typedef enum {
    HEALTH,
    SPEED,
    STAMINA,
    INVISIBILITY,
    TIME_FREEZE,
    CONSUMABLE_COUNT,
    NONE
} ConsumableType;

typedef enum {
    RED,
    GREEN,
    BLUE,
    YELLOW,
    CRAFTABLE_COUNT,
    CRAFT_NONE
} CraftType;

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

void inventory_add_craft(CraftType type, Uint8 count);
void inventory_try_craft(CraftType item_1, CraftType item_2);

Sprite* inventory_manager_consumables_img();
Sprite* inventory_manager_craftables_img();

void inventory_show_consumables();
void inventory_clear();

void inventory_load(int consumables[CONSUMABLE_COUNT], int craftables[CRAFTABLE_COUNT]);
void inventory_save(SJson* save);

#endif