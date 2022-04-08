#ifndef __PG_ENTITY_H__
#define __PG_ENTITY_H__
#include <SDL.h>
#include "rect.h"
#include "gf2d_sprite.h"

typedef struct ENT_S {
    Uint8 _inuse;          /**<is entity in use */ 
    
    Uint8 visible;
    Sprite* sprite;        /**<sprite used to draw this entity */
    float frame;           /**<current anim frame */
    
    int health;         /**<current entity health */
    struct ENT_S* owner;   /**<who is this entity owned by? */

    Vector2D draw_offset;  /**<draw position relative to ent position */
    Vector2D position;     /**<position of this entity */
    Vector2D velocity;     /**<where our entity is going */
    Vector3D rotation;     /**<rotation of this entity*/
    Vector2D scale;
    
    Vector2D flip;

    Rect hurtbox;

    //This forces 32B alignment so there arent any isssues with casting it
    union {
        Uint8 data[32];  /**<extra entity data (can be cast depending on expected type) */
        intmax_t alignment_force;
    };

    void (*think)(struct ENT_S* self);
    void (*cleanup)(struct ENT_S* self);

} Entity;


/**
 * @brief initialize entity list
 * @note must be called before all other entity functions
 * 
*/
void entity_manager_init(Uint32 max_entities);

void entity_manager_close();

void entity_manager_think_all();

/**
 * @brief free all active entities
 * 
*/
void entity_manager_clear();

/**
 * @brief get a new empty entity
 * @returns NULL on error or a pointer to an entity
 * 
*/
Entity* entity_new();


/**
 * @brief draw an entity on screen
 * 
*/
void entity_draw(Entity* ent);

void entity_manager_draw_entities();

/**
 * @brief free an active entity
 * 
*/
void entity_free(Entity *ent);

/**
 * @brief damage all entities in the given radus
 * 
*/
void damage_radius(Entity* ignore, Vector2D p, Uint8 damage, float range);

/**
 * @brief returns the first entity the input collides with
 * 
*/
Entity* get_colliding(Entity* ent);
Entity* get_colliding_r(Rect r, Entity* filter);

/**
 * @brief sets the entity manager's fast access player field to point at this entity
 * 
*/
void entity_manager_set_player(Entity* e);

/**
 * @brief returns the entity in this entity manager marked as the player
 * 
*/
Entity* entity_manager_get_player();


void entity_manager_time_freeze();

void entity_manager_time_unfreeze();

void entity_manager_toggle_draw_debug();

void entity_manager_set_draw_debug(Uint8 draw);

#endif