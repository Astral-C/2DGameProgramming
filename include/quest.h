#ifndef __QUEST_H__
#define __QUEST_H__
#include <SDL.h>
#include "simple_json.h"
#include "gfc_hashmap.h"
#include "simple_logger.h"

typedef enum {
    ET_KillEnemy,
    ET_EnableFlag
} EventType;

typedef enum {
    RWT_CASH,
    RWT_ROOT,
    RWT_POTION
} QuestRewardType;

typedef struct {
    Uint8 progress;
    Uint8 completion_progress;
    Uint8 is_complete;
    Uint8 reward_given;
    Uint8 reward_type;
    Uint8 reward_value_1;
    Uint8 reward_value_2;
    EventType type;
    int tag;

    TextLine name;
    TextLine id;
    TextBlock description;
} Quest;

typedef struct { 
    Quest* active_quest;
    List* registered_quest_ids;
    HashMap* available_quests;
} QuestManager;

void quest_manager_init();
void quest_manager_free();
void activate_quest(char* quest_name);
void quest_manager_notify(EventType type, int tag);
Uint8 quest_manager_check_reward(char* quest_name);
Uint8 quest_manager_check_completion(char* quest_name);
void add_quest(char* id, char* name, char* description, EventType type, int tag, Uint8 completion_progress, Uint8 reward_type, Uint8 reward_value_1, Uint8 reward_value_2);

QuestManager* get_quest_manager();

#endif