#ifndef __QUEST_H__
#define __QUEST_H__
#include <SDL.h>
#include <simple_json.h>
#include <gfc_hashmap.h>

typedef enum {
    ET_KillEnemy,
    ET_EnableFlag
} EventType;

typedef struct {
    Uint8 progress;
    Uint8 completion_progress;
    Uint8 is_complete;
    int tag;
    EventType type;
} Quest;

typedef struct { 
    Quest* active_quest;
    HashMap* available_quests;
} QuestManager;

void activate_quest(char* quest_name);
void quest_manager_notify(EventType type, int tag);
Uint8 quest_manager_check_completion(char* quest_name);

#endif