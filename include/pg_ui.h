#ifndef __UI_H__
#define __UI_H__
#include "gf2d_sprite.h"
#include "gfc_text.h"
#include "pg_entity.h"
#include <SDL_ttf.h>

typedef enum {
    STRING,
    IMAGE,
    PROGRESS
} UIType;

typedef struct {
    TextLine str[2];
    Sprite* img;
    struct {
        int max;
        int* current;
    };
} UIValue;


typedef struct UIELEMENT_S {
    Uint8 _inuse;
    Uint8 hidden;
    
    float frame; //in the event that this ui element is animated 
    UIType type;
    Vector2D scale;
    Vector2D position;
    
    Entity* follow;
    SDL_Rect _transform; //generated from position and scale on render. Slow!
    UIValue value;
} UIElement;

UIElement* ui_element_new();
void update_ui_element(UIElement* element, UIType type, UIValue value);
void draw_ui_element(UIElement* element);

/* Init's ui system with custom font */
void ui_manager_init_font(Uint32 max_elements, char* font_path);

/* Init's ui system with default font */
void ui_manager_init_default(Uint32 max_elements);

/* UI Element add functions */
UIElement* ui_manager_add_text(Vector2D location, char* string);
UIElement* ui_manager_add_image(Vector2D location, char* path, Uint32 w, Uint32 h, Uint32 frames);
UIElement* ui_manager_add_health(Entity* follow, int max);

/* Generic render text function usable by anything */
Sprite* ui_manager_render_text(char* string, SDL_Color color);

void ui_manager_free(UIElement* element);
void ui_manager_clear();
void ui_manager_close();
void ui_manager_draw();

#endif