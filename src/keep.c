#include "simple_logger.h"
#include "simple_json.h"

#include "level.h"
#include "keep.h"

static char *_diretion_names[] = 
{
    "s",
    "sw",
    "w",
    "nw",
    "n",
    "ne",
    "e",
    "se"
};

static char *_stage_names[] = 
{
    "base",
    "start",
    "half",
    "nearly",
    "finished"
};

char *keep_get_direction_name(KeepDirection direction)
{
    if (direction >= KD_MAX)return NULL;
    return _diretion_names[direction];
}

char *keep_get_state_name(KeepStage stage)
{
    if (stage >= KS_MAX)return NULL;
    return _stage_names[stage];
}

Entity *keep_segment_new(Vector2D position,const char *segment,KeepStage stage, KeepDirection direction)
{
    Entity *ent;
    Level *level;
    ent = gf2d_entity_new();
    if (!ent)return NULL;
        
    level = level_get_current();
    
    gf2d_actor_load(&ent->actor,(char *)segment);
    gf2d_actor_set_action(&ent->actor,keep_get_direction_name(direction));
    ent->actor.frame += stage;
    vector2d_copy(ent->position,position);
    
    if (level)
    {
        ent->position.x += level->levelTileSize.x/2 - ent->actor.sprite->frame_w/2;
        ent->position.y += level->levelTileSize.y/2 - ent->actor.sprite->frame_h;
    }
    
    return ent;
}




/*eol@eof*/
