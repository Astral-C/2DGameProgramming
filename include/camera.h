#ifndef __CAM_H__
#define __CAM_H__
#include "gfc_vector.h"
#include "pg_entity.h"


struct Camera {
    Vector2D position;
    Vector2D accel;
    Vector2D zoom;
    float move_border_x;
    float move_border_y;
    Entity* target;
    SDL_Rect screen_rect;
};

void init_camera(float move_border_x, float move_border_y);
void set_camera_target(Entity* ent);
void update_camera();

Vector2D get_camera_pos();
Vector2D* get_camera_zoom();


#endif