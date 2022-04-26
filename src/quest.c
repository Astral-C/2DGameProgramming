#include "quest.h"
#include "player.h"
#include "inventory.h"

static QuestManager quests = {0};

void quest_manager_init(){
    quests.available_quests = gfc_hashmap_new();
    quests.registered_quest_ids = gfc_list_new();

    atexit(quest_manager_free);
}

void quest_manager_free(){
    gfc_hashmap_free(quests.available_quests);
    gfc_list_delete(quests.registered_quest_ids);
}

void add_quest(char* id, char* name, char* description, EventType type, int tag, Uint8 completion_progress, Uint8 reward_type, Uint8 reward_value_1, Uint8 reward_value_2){
    Quest* q = malloc(sizeof(Quest));
    q->tag = tag;
    q->type = type;
    q->completion_progress = completion_progress;
    q->is_complete = 0;
    q->progress = 0;

    if(reward_type != 0xFF){
        q->reward_given = 0;
        q->reward_type = reward_type;
        q->reward_value_1 = reward_value_1;
        q->reward_value_2 = reward_value_2;
    } else {
        q->reward_given = 1;
    }

    gfc_line_cpy(q->id, id);
    gfc_line_cpy(q->name, name);
    gfc_block_cpy(q->description, description);

    gfc_list_append(quests.registered_quest_ids, q->id);

    gfc_hashmap_insert(quests.available_quests, id, q);
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
    
    if(!quests.active_quest) return 0xFF;
    
    if(!q){
        return 0xFF;
    }

    return q->is_complete;
}

Uint8 quest_manager_check_reward(char* quest_name){
    Quest* q;
    q = gfc_hashmap_get(quests.available_quests, quest_name); 
    
    if(!q){
        return 0xFF;
    }

    if(!q->reward_given){
        q->reward_given = 1;
        if(q->reward_type & RWT_CASH){
            player_add_money(q->reward_value_1);
        }
        if(q->reward_type & RWT_POTION){
            inventory_add_consumable(q->reward_value_1, q->reward_value_2);
        }
        if(q->reward_type & RWT_ROOT){
            inventory_add_craftable(q->reward_value_1, q->reward_value_2);
        }
    }

    return 1; //why?
}

QuestManager* get_quest_manager(){
    return &quests;
}

void quest_manager_load(SJson* quests){
    
}