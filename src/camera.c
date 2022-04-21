#include "camera.h"
#include "gfc_input.h"
#include "gf2d_graphics.h"
#include "simple_logger.h"
#include "map.h"

#define cam_speed 0.8f //this shuld be a var but for now

struct Camera cam = {0};

void init_camera(float move_border_x, float move_border_y){
    cam.move_border_x = move_border_x; // within 40 units of screen border makes the camera move
    cam.move_border_y = move_border_y;
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
    if(cam.target == NULL){
        if(gfc_input_command_down("left")){
            cam.position.x += -5;
        } else if (gfc_input_command_down("right")){
            cam.position.x += 5;
        } else if(gfc_input_command_down("stomp")){
            cam.position.y += 5;
        } else if(gfc_input_command_down("jump")){
            cam.position.y -= 5;
        }
    } else {

        //TODO: add a speed modifier to change how the camera movement scales?
        if((cam.target->position.x) < (cam.position.x + cam.move_border_x) && cam.position.x > 0){ //todo: restrict to map width
            cam.accel.x = ((cam.target->position.x) - (cam.position.x + cam.move_border_x)) * cam_speed;
        }

        if((cam.target->position.x) > (cam.position.x + cam.screen_rect.w) - cam.move_border_x){
            cam.accel.x = ((cam.target->position.x) - ((cam.position.x + cam.screen_rect.w) - cam.move_border_x)) * cam_speed;
        }

        if(abs(cam.target->position.y - cam.position.y) < cam.move_border_y && cam.position.y > 0){
            cam.accel.y = ((cam.target->position.y) - (cam.position.y + cam.move_border_y)) * cam_speed;
        }

        if(abs(cam.target->position.y - cam.position.y) > cam.screen_rect.h - cam.move_border_y){
            cam.accel.y = ((cam.target->position.y) - ((cam.position.y + cam.screen_rect.h) - cam.move_border_y)) * cam_speed;
        }


        vector2d_add(cam.position, cam.position, cam.accel);
    }
    if(cam.position.x < 0){
        cam.position.x = 0;
        cam.accel.x = 0;
    }
    if((cam.position.x + 1200) > current_map_width()){
        cam.position.x = current_map_width() - 1200;
        cam.accel.x = 0;
    }

    if(cam.position.y < 0){
        cam.position.y = 0;
        cam.accel.y = 0;
    }
    if((cam.position.y + 720) > current_map_height()){
        cam.position.y = current_map_height() - 720;
        cam.accel.y = 0;
    }

    if(cam.accel.x != 0){
        if(cam.accel.x < 0) cam.accel.x = (cam.accel.x + 0.5 > 0 ? 0.0f : cam.accel.x + 0.5);
        if(cam.accel.x > 0) cam.accel.x = (cam.accel.x - 0.5 > 0 ? 0.0f : cam.accel.x - 0.5);
    }

    if(cam.accel.y != 0){
        if(cam.accel.y < 0) cam.accel.y = (cam.accel.y + 0.5 > 0 ? 0.0f : cam.accel.y + 0.5);
        if(cam.accel.y > 0) cam.accel.y = (cam.accel.y - 0.5 > 0 ? 0.0f : cam.accel.y - 0.5);
    }

}