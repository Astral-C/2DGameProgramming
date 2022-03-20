#include "camera.h"
#include "gf2d_graphics.h"
#include "simple_logger.h"

#define cam_speed 0.8f //this shuld be a var but for now

struct Camera cam = {0};

void init_camera(float move_border){
    cam.move_border = move_border; // within 40 units of screen border makes the camera move
    cam.position = vector2d(0, 0);
    cam.accel = vector2d(0, 0);
    cam.zoom = vector2d(4,4);
    SDL_RenderGetViewport(gf2d_graphics_get_renderer(), &cam.screen_rect);
}

void set_camera_target(Entity* ent){
    cam.target = ent;
}

Vector2D get_camera_pos(){
    return cam.position; 
}

Vector2D* get_camera_zoom(){
    return &cam.zoom; 
}

void update_camera(){
    //TODO: add a speed modifier to change how the camera movement scales?
    if((cam.target->position.x) < (cam.position.x + cam.move_border) && cam.position.x > 0){ //todo: restrict to map width
        cam.accel.x = ((cam.target->position.x) - (cam.position.x + cam.move_border)) * cam_speed;
    }
    
    if((cam.target->position.x) > (cam.position.x + cam.screen_rect.w) - cam.move_border){
        cam.accel.x = ((cam.target->position.x) - ((cam.position.x + cam.screen_rect.w) - cam.move_border)) * cam_speed;
    }

    if(abs(cam.target->position.y - cam.position.y) < cam.move_border && cam.position.y > 0){
        //cam.accel.x -= 5; 
    }

    if(abs(cam.target->position.y - cam.position.y) > cam.screen_rect.h - cam.move_border){
        //cam.accel.x += 5;
    }

    vector2d_add(cam.position, cam.position, cam.accel);

}