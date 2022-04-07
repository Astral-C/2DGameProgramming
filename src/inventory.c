#include "inventory.h"
#include "gfc_list.h"
#include "pg_ui.h"

typedef struct {
    Uint8 consumables[CONSUMABLE_COUNT];
    Sprite* consumable_counts[CONSUMABLE_COUNT];
    Sprite* consumable_icons;
    List* items;
} InventoryManager;

static InventoryManager inventory = {0};

void inventory_init(){
    char num[3];
    inventory.consumable_icons = gf2d_sprite_load_all("images/potions.png", 128, 128, 5);
    inventory.items = gfc_list_new();
    
    for (size_t i = 0; i < CONSUMABLE_COUNT; i++){
        inventory.consumables[i] = 3;
        snprintf(num, 3, "%d", inventory.consumables[i]);
        inventory.consumable_counts[i] = ui_manager_render_text(num, (SDL_Color){255, 255, 255});
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

void inventory_clear(){
    char num[3];
    
    for (size_t i = 0; i < CONSUMABLE_COUNT; i++){
        inventory.consumables[i] = 0;

        gf2d_sprite_free(inventory.consumable_counts[i]);

        snprintf(num, 3, "%d", inventory.consumables[i]);
        inventory.consumable_counts[i] = ui_manager_render_text(num, (SDL_Color){255, 255, 255});
    }
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
