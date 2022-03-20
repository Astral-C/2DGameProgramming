#ifndef __MAP_H__
#define __MAP_H__
#include "gf2d_sprite.h"
#include "gfc_list.h"
#include "rect.h"
#include "pg_entity.h"

typedef enum {
    POINT,
    RECT,
    CIRCLE
} ColType;

typedef struct MAP_S {
    Uint8 _ref_count; //what the fuck was I doing here?
    Uint8 is_town;
    Sprite* background;
    Sprite* decoration;
    Sprite* foreground;
 
    Uint32 rect_count;
    Rect* collision;
    
    TextLine name; //TODO: replace this with a hash?


} Map;

//TODO: Write free func

void map_manager_init(Uint8 map_count);
Map* map_new(char* name);

void set_map_active();
Map* get_map_active();

Uint8 collide_worldp(Vector2D p);
Uint8 ent_collide_world(Entity* ent);

void map_draw_fg(Map* m);
void map_draw_bg(Map* m);

void map_manager_draw_fg();
void map_manager_draw_bg();

#endif