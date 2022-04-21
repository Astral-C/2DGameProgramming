#include "inventory.h"
#include "gfc_list.h"
#include "pg_ui.h"

typedef struct {
    Uint8 consumables[CONSUMABLE_COUNT];
    Uint8 craftables[CRAFTABLE_COUNT];
    Sprite* consumable_counts[CONSUMABLE_COUNT];
    Sprite* consumable_icons;
    List* items;
} InventoryManager;

static InventoryManager inventory = {0};

void inventory_init(){
    char num[3];
    inventory.consumable_icons = gf2d_sprite_load_all("images/potions.png", 128, 128, 5);
    //todo: load craftables
    inventory.items = gfc_list_new(); //fucking what??
    
    for (size_t i = 0; i < CONSUMABLE_COUNT; i++){
        inventory.consumables[i] = 3;
        snprintf(num, 3, "%d", inventory.consumables[i]);
        inventory.consumable_counts[i] = ui_manager_render_text(num, (SDL_Color){255, 255, 255});
    }
    
    for (size_t i = 0; i < CRAFTABLE_COUNT; i++){
        inventory.craftables[i] = 3;
    }

    atexit(inventory_cleanup);
}

void inventory_cleanup(){
    if(inventory.consumable_icons) gf2d_sprite_free(inventory.consumable_icons);
    gfc_list_delete(inventory.items);
}

void inventory_manager_save(char* path){

}

void inventory_manager_load(char* path){

}

void inventory_add_consumable(ConsumableType type, Uint8 count){
    char num[3];
    
    inventory.consumables[type] = (inventory.consumables[type] + count > 99 ? 99: inventory.consumables[type] + count);
    
    gf2d_sprite_free(inventory.consumable_counts[type]);

    snprintf(num, 3, "%d", inventory.consumables[type]);
    inventory.consumable_counts[type] = ui_manager_render_text(num, (SDL_Color){255, 255, 255});
}

Uint8 inventory_use_consumable(ConsumableType type){
    char num[3];
    
    if(inventory.consumables[type] == 0) return 0;
    inventory.consumables[type]--;

    gf2d_sprite_free(inventory.consumable_counts[type]);

    snprintf(num, 3, "%d", inventory.consumables[type]);
    inventory.consumable_counts[type] = ui_manager_render_text(num, (SDL_Color){255, 255, 255});
    return 1;
}


void inventory_add_craftable(CraftType type, Uint8 count){
    inventory.craftables[type] = (inventory.craftables[type] + count > 99 ? 99: inventory.craftables[type] + count);
}

void inventory_try_craft(CraftType item_1, CraftType item_2){
    if(item_1 == RED && item_2 == BLUE){
        //add a potion
    }
}

void inventory_clear(){
    char num[3];
    
    for (size_t i = 0; i < CONSUMABLE_COUNT; i++){
        inventory.consumables[i] = 0;

        gf2d_sprite_free(inventory.consumable_counts[i]);

        snprintf(num, 3, "%d", inventory.consumables[i]);
        inventory.consumable_counts[i] = ui_manager_render_text(num, (SDL_Color){255, 255, 255});
    }

    for (size_t i = 0; i < CRAFTABLE_COUNT; i++){
        inventory.craftables[i] = 0;
    }

    gfc_list_delete(inventory.items);
}

void inventory_load(int consumables[CONSUMABLE_COUNT], int craftables[CRAFTABLE_COUNT]){
    int i;
    char num[3];
    for (i = 0; i < CONSUMABLE_COUNT; i++){
        inventory.consumables[i] = consumables[i];
        gf2d_sprite_free(inventory.consumable_counts[i]);
        snprintf(num, 3, "%d", inventory.consumables[i]);
        inventory.consumable_counts[i] = ui_manager_render_text(num, (SDL_Color){255, 255, 255});
    }

    for (i = 0; i < CRAFTABLE_COUNT; i++){
        inventory.craftables[i] = craftables[i];
    }
    
}

void inventory_save(SJson* save){
    int i;
    SJson *consumables, *craftables;
    craftables = sj_array_new();
    consumables = sj_array_new();

    for (i = 0; i < CONSUMABLE_COUNT; i++){
        sj_array_append(consumables, sj_new_int(inventory.consumables[i]));
    }

    for (i = 0; i < CRAFTABLE_COUNT; i++){
        sj_array_append(craftables, sj_new_int(inventory.craftables[i]));
    }

    sj_object_insert(save, "consumables", consumables);
    sj_object_insert(save, "craftables", craftables);
}

void inventory_show_consumables(){
    Vector2D cursor = vector2d(16, 64);
    Vector2D text_cursor = vector2d(70, 96);
    Vector2D scale = vector2d(0.5, 0.5);
    for (size_t i = 0; i < CONSUMABLE_COUNT; i++){
        gf2d_sprite_draw(inventory.consumable_icons, cursor, &scale, NULL, NULL, NULL, NULL, i);
        gf2d_sprite_draw_image(inventory.consumable_counts[i], text_cursor);

        cursor.y += 64;
        text_cursor.y += 64;
    }
}

Sprite* inventory_manager_consumables_img(){
    return inventory.consumable_icons;
}
