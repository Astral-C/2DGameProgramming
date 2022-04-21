#include "quest.h"

static QuestManager quests = {0};

void quest_manager_init(){
    quests.available_quests = gfc_hashmap_new();

    atexit(quest_manager_free);
}

void quest_manager_free(){
    gfc_hashmap_free(quests.available_quests);
}

void add_quest(char* name, EventType type, int tag, Uint8 completion_progress){
    Quest* q = malloc(sizeof(Quest));
    q->tag = tag;
    q->type = type;
    q->completion_progress = completion_progress;
    q->is_complete = 0;
    q->progress = 0;
    gfc_line_cpy(q->name, name);

    gfc_hashmap_insert(quests.available_quests, name, q);
}

void activate_quest(char* quest_name){
    quests.active_quest = gfc_hashmap_get(quests.available_quests, quest_name);
}

void quest_manager_notify(EventType type, int tag){
    if(quests.active_quest != NULL && !quests.active_quest->is_complete && type == quests.active_quest->type){
        switch (type)
        {
        case ET_EnableFlag:
            if(quests.active_quest->completion_progress == tag) quests.active_quest->is_complete = 1;
            break;
        
        case ET_KillEnemy:
            //slog("Updating Quests...");
            if(tag == quests.active_quest->tag){
                quests.active_quest->progress++;
                //slog("Killed enemy with correct tag, progress is %d", quests.active_quest->progress);
                if(quests.active_quest->progress >= quests.active_quest->completion_progress){
                    quests.active_quest->is_complete = 1;
                }
            }   
        default:
            break;
        }     
    }
}

Uint8 quest_manager_check_completion(char* quest_name){
    Quest* q;
    q = gfc_hashmap_get(quests.available_quests, quest_name); 
    
    if(!q){
        return 0xFF;
    }

    return q->is_complete;
}


QuestManager* get_quest_manager(){
    return &quests;
}

void quest_manager_load(SJson* quests){
    
}