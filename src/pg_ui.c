#include "pg_ui.h"
#include "gf2d_graphics.h"
#include "simple_logger.h"
#include "camera.h"
#include <SDL_ttf.h>

#define CHAR_FRAME(v) (v > 97 && v < 122 ? v - 97 : (v > 65 && v < 90 ? (v - 65) + 26 : -1))

typedef struct {
    Uint32 max_elements;
    UIElement *ui_elements;
    Sprite* font;
    float timer;
    Sprite* progress_border;
    Sprite* progress_bar;
    TTF_Font* ttf_font;
} UIManager;

static UIManager ui_manager = {0};

void ui_manager_init_font(Uint32 max_elements, char* font_path){
    if(max_elements == 0){
        slog("Cannot allocate memory for 0 elemetnts");
        return;
    }

    if(ui_manager.ui_elements != NULL){
        slog("ui manager already intitialized");
        return;
    }

    ui_manager.max_elements = max_elements;
    ui_manager.ui_elements = gfc_allocate_array(sizeof(UIElement), max_elements);

    atexit(TTF_Quit);
    atexit(ui_manager_close);
}

void ui_manager_init_default(Uint32 max_elements){
    if(max_elements == 0){
        slog("Cannot allocate memory for 0 elemetnts");
        return;
    }

    if(ui_manager.ui_elements != NULL){
        slog("ui manager already intitialized");
        return;
    }

    ui_manager.max_elements = max_elements;
    ui_manager.ui_elements = gfc_allocate_array(sizeof(UIElement), max_elements);

    TTF_Init();

    ui_manager.ttf_font = TTF_OpenFont("HelvetiPixel.ttf", 32);
    ui_manager.font = gf2d_sprite_load_all("images/font.png", 7, 14, 67);
    ui_manager.progress_border = gf2d_sprite_load_image("images/progress_ext.png");
    ui_manager.progress_bar = gf2d_sprite_load_image("images/progress_inner.png");

    //atexit(TTF_Quit);
    atexit(ui_manager_close);
}

UIElement* ui_element_new(){
    int i;
    for(i=0;i<ui_manager.max_elements;i++){
        if(!ui_manager.ui_elements[i]._inuse){
            ui_manager.ui_elements[i]._inuse = 1;
            return &ui_manager.ui_elements[i];
        }
    }

    slog("All elements in use");
    return NULL;
}

void draw_ui_element(UIElement* element){
    if(element == NULL) return;
    if(element->value.img == NULL && element->type == IMAGE) return;

    if(element->type == IMAGE){
        gf2d_sprite_draw(element->value.img, element->position, &element->scale, NULL, NULL, NULL, NULL, 0);   
    }
    if(element->type == STRING){
        Vector2D cursor;
        vector2d_copy(cursor, element->position);
        for (size_t i = 0; i < 128; i++){
            cursor.y = element->position.y - sin(i + ui_manager.timer)*10.f;
            int frame = CHAR_FRAME(element->value.str[0][i]);
            if(frame >= 0) gf2d_sprite_draw(ui_manager.font, cursor, &element->scale, NULL, NULL, NULL, NULL, CHAR_FRAME(element->value.str[0][i])); 
            cursor.x += 8*4;
        }
        
    }
    if(element->type == HEALTHBAR){
        element->position.x = element->follow->position.x;
        element->position.y = element->follow->position.y + 64;
        vector2d_sub(element->position, element->position, get_camera_pos());
        Vector2D bar_inner = vector2d((float)*element->value.icurrent / (float)element->value.imax, 1.0f);
        gf2d_sprite_draw(ui_manager.progress_bar, element->position, &bar_inner, NULL, NULL, NULL, &element->color, 0);
        gf2d_sprite_draw(ui_manager.progress_border, element->position, NULL, NULL, NULL, NULL, NULL, 0);
    }
    if(element->type == PROGRESS){
        Vector2D bar_scale = vector2d(3,3);
        Vector2D bar_inner = vector2d(((float)*element->value.fcurrent / (float)element->value.fmax) * 3, 3.0f);
        gf2d_sprite_draw(ui_manager.progress_bar, element->position, &bar_inner, NULL, NULL, NULL, &element->color, 0);
        gf2d_sprite_draw(ui_manager.progress_border, element->position, &bar_scale, NULL, NULL, NULL, NULL, 0);
    }
}

UIElement* ui_manager_add_text(Vector2D location, char* string){
    UIElement* e = ui_element_new();
    if(e == NULL){
        //log
        return NULL;
    }

    e->type = STRING;

    vector2d_copy(e->position, location);
    e->scale = vector2d(4,4);

    gfc_line_cpy(e->value.str[0], string);

    return e;
}

Sprite* ui_manager_render_text(char* string, SDL_Color color){
    Sprite* img = gf2d_sprite_new();

    //slog("loaded text %s", e->value.str);

    SDL_Surface* text_surface = TTF_RenderText_Solid(ui_manager.ttf_font, string, color);
    if(text_surface == NULL){
        //slog("couldnt render text %s", string;
        gf2d_sprite_free(img);
        return NULL;
    }

    img->frame_w = text_surface->w;
    img->frame_h = text_surface->h;
    img->texture = SDL_CreateTextureFromSurface(gf2d_graphics_get_renderer(), text_surface);
    SDL_SetTextureBlendMode(img->texture, SDL_BLENDMODE_BLEND);       
    SDL_SetTextureScaleMode(img->texture, SDL_ScaleModeNearest);

    SDL_FreeSurface(text_surface);
    return img;
}

UIElement* ui_manager_add_image(Vector2D location, char* path, Uint32 w, Uint32 h, Uint32 frames){
    UIElement* e = ui_element_new();
    if(e == NULL){
        //log
        return NULL;
    }

    vector2d_copy(e->position, location);
    e->value.img = gf2d_sprite_load_all(path, w, h, frames);
    e->scale = vector2d(4,4);
    e->type = IMAGE;

    return e;
}

void ui_manager_draw(){
    int i;
    for(i=0;i<ui_manager.max_elements;i++){
        if(ui_manager.ui_elements[i]._inuse){
            draw_ui_element(&ui_manager.ui_elements[i]);
        }
    }
    ui_manager.timer += 0.1f;
}

UIElement* ui_manager_add_health(Entity* follow, int max){
    UIElement* e = ui_element_new();
    if(e == NULL){
        //log
        return NULL;
    }

    e->follow = follow;
    e->value.imax = max;
    e->value.icurrent = &follow->health;
    e->type = HEALTHBAR;
    e->color = vector4d(0x50, 0x80, 0x60, 0xFF);

    return e; 
}

UIElement* ui_manager_add_progressf(Vector2D pos, Vector4D color, float max, float* track){
    UIElement* e = ui_element_new();
    if(e == NULL){
        //log
        return NULL;
    }

    e->position = pos;
    e->follow = NULL;
    e->value.fmax = max;
    e->value.fcurrent = track;
    e->type = PROGRESS;
    e->color = color;

    return e;  
}

UIElement* ui_manager_add_progressi(Vector2D pos, Vector4D color, int max, int* track){
    UIElement* e = ui_element_new();
    if(e == NULL){
        //log
        return NULL;
    }

    e->position = pos;
    e->follow = NULL;
    e->value.imax = max;
    e->value.icurrent = track;
    e->type = PROGRESS;
    e->color = color;

    return e;  
}


void ui_manager_close(){
    ui_manager_clear();

    gf2d_sprite_free(ui_manager.font);

    if(ui_manager.ui_elements != NULL){
        free(ui_manager.ui_elements);
    }
}

void ui_manager_clear(){
    int i;
    for(i=0;i<ui_manager.max_elements;i++){
        if(!ui_manager.ui_elements[i]._inuse) continue;
        ui_manager_free(&ui_manager.ui_elements[i]);
    }
}

void ui_manager_clear_ephemeral(){
    int i;
    for(i=0;i<ui_manager.max_elements;i++){
        if(!ui_manager.ui_elements[i]._inuse || ui_manager.ui_elements[i].type != HEALTHBAR) continue;
        ui_manager_free(&ui_manager.ui_elements[i]);
    } 
}

void ui_manager_free(UIElement* element){
    if(element == NULL){
        return;
    }
    if(element->value.img != NULL) gf2d_sprite_free(element->value.img);

    memset(element, 0, sizeof(UIElement));
}