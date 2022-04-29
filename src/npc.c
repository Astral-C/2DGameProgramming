#include "simple_json.h"
#include "npc.h"
#include "gfc_input.h"
#include "simple_logger.h"
#include "gf2d_graphics.h"
#include "inventory.h"
#include "player.h"
#include "camera.h"
#include "quest.h"
#include "menu.h"

static InteractionManager interaction_manager = {0};

void npc_think(Entity* self){
    SJson* quest;
    char* quest_id;
    Entity* p = entity_manager_get_player();
    NpcProps* props = (NpcProps*)self->data;

    if(props->has_quest){
        quest = sj_object_get_value(props->npc_json, "quest");
        quest_id = (char*)sj_get_string_value(sj_object_get_value(quest, "id"));
    }

    if(!interaction_manager.interacting && interaction_manager.requested_interaction && rect_collider(self->hurtbox, p->hurtbox)){
        slog("interacting beginning");
        interaction_manager.interacting = self;
        interaction_manager.requested_interaction = 0;
        interaction_manager.current_page = 0;

        gf2d_sprite_free(props->textbox_text);

        char* page_text = "";
        if(!props->has_quest || (props->has_quest && quest_manager_check_completion(quest_id) == 1)){
            quest_manager_check_reward(quest_id);
            page_text = (char*)sj_get_string_value(sj_array_get_nth(sj_object_get_value(props->npc_json, "pages"), interaction_manager.current_page % props->textbox_pages));
        } else {
            if(props->has_quest && quest_manager_check_completion(quest_id) == 0){
                page_text = (char*)sj_get_string_value(sj_object_get_value(quest, "progress_msg"));
            } else {
                page_text = (char*)sj_get_string_value(sj_object_get_value(quest, "incomplete_text"));
            }
        }
        props->textbox_text = ui_manager_render_text(page_text, (SDL_Color){0xFF, 0xFF, 0xFF, 0xFF});
        return;
    }

    if(interaction_manager.interacting != self) return;

    if(props->is_shop){
        if(gfc_input_command_released("switch_weapon")){
            interaction_manager.interacting = NULL;
        }

        if(gfc_input_command_released("fire_weapon")){
            if(player_spend_money(props->sell_price)){
                inventory_add_consumable(props->sell_type, 1);
            }
        }
    } else if(!props->is_shop && props->has_quest && quest_manager_check_completion(quest_id) == 0xFF) {
        EventType type;
        char* quest_name, *quest_description;
        int tag, complete, reward_type, reward_value_1, reward_value_2;

        sj_get_integer_value(sj_object_get_value(quest, "type"), (int*)&type);
        sj_get_integer_value(sj_object_get_value(quest, "tag"), &tag);
        sj_get_integer_value(sj_object_get_value(quest, "complete"), &complete);

        SJson* rwt = sj_object_get_value(quest, "reward_type");
        if(rwt != NULL){
            sj_get_integer_value(sj_object_get_value(quest, "reward_type"), &reward_type);
            sj_get_integer_value(sj_object_get_value(quest, "reward_value_1"), &reward_value_1);
            sj_get_integer_value(sj_object_get_value(quest, "reward_value_2"), &reward_value_2);
        } else {
            reward_type = 0xFF;
            reward_value_1 = 0xFF;
            reward_value_2 = 0xFF;
        }

        quest_name = (char*)sj_get_string_value(sj_object_get_value(quest, "name"));
        quest_description = (char*)sj_get_string_value(sj_object_get_value(quest, "description"));

        if(gfc_input_command_released("switch_weapon")){
            interaction_manager.interacting = NULL;
            return;
        } else if (gfc_input_command_released("fire_weapon")) {
            if(quest_manager_check_completion(quest_name) == 0xFF){
                add_quest(quest_id, quest_name, quest_description, type, tag, complete, reward_type, reward_value_1, reward_value_2);
                activate_quest(quest_id);
            }
            interaction_manager.interacting = NULL;
            return;
        }

    } else if (props->has_quest && quest_manager_check_completion(quest_id) == 0){
        if(gfc_input_command_released("fire_weapon")){
            interaction_manager.interacting = NULL;
            return;
        }  
    } else {
        if(gfc_input_command_released("fire_weapon")){
            if(interaction_manager.current_page >= props->textbox_pages - 1){
                interaction_manager.interacting = NULL;
                return;
            }
            interaction_manager.current_page++;
            gf2d_sprite_free(props->textbox_text);
            const char* page_text = sj_get_string_value(sj_array_get_nth(sj_object_get_value(props->npc_json, "pages"), interaction_manager.current_page));
            props->textbox_text = ui_manager_render_text((char*)page_text, (SDL_Color){0xFF, 0xFF, 0xFF, 0xFF});

        }

    }
}

void npc_cleanup(Entity* self){
    NpcProps* props = (NpcProps*)self->data;
    if(props->npc_json){
        sj_free(props->npc_json);
    }
    if(props->textbox_text) gf2d_sprite_free(props->textbox_text);
}

Entity* npc_new(char* path){

    int is_shop, has_quest;

    Entity* npc = entity_new();
    npc->type = ENT_NPC;
    NpcProps* props = (NpcProps*)npc->data;

    SJson* npc_def = sj_load(path);
    if(!npc) return NULL; //oops

    props->npc_json = npc_def;
    sj_object_insert(npc_def, "npc_path", sj_new_str(path));

    npc->think = npc_think;
    npc->cleanup = npc_cleanup;

    SJson* rect = sj_object_get_value(npc_def, "interact");
    
    int x, y, w, h;
    sj_get_integer_value(sj_array_get_nth(rect, 0), &x);
    sj_get_integer_value(sj_array_get_nth(rect, 1), &y);
    sj_get_integer_value(sj_array_get_nth(rect, 2), &w);
    sj_get_integer_value(sj_array_get_nth(rect, 3), &h);
    
    npc->hurtbox.pos.x = x << 2;
    npc->hurtbox.pos.y = y << 2;
    npc->hurtbox.size.x = w << 2;
    npc->hurtbox.size.y = h << 2;
    
    npc->position.x = x << 2;
    npc->position.y = y << 2;
    
    char* img_path = (char*)sj_get_string_value(sj_object_get_value(npc_def, "img"));

    npc->sprite = gf2d_sprite_load_image(img_path);
    npc->scale = vector2d(1,1);


    sj_get_integer_value(sj_object_get_value(npc_def, "is_shop"), &is_shop);
    props->is_shop = is_shop;

    sj_get_integer_value(sj_object_get_value(npc_def, "has_quest"), &has_quest);
    props->has_quest = has_quest;
    
    props->textbox_pages = sj_array_get_count(sj_object_get_value(npc_def, "pages"));

    SJson* sell_item = sj_object_get_value(npc_def, "item");
    int type, price;
    if(sell_item){
        sj_get_integer_value(sj_array_get_nth(sell_item, 0), &type);
        sj_get_integer_value(sj_array_get_nth(sell_item, 1), &price);
    }
    
    props->sell_type = type;
    props->sell_price = price;

    return npc;
}

Uint8 npc_request_interaction(){
    interaction_manager.requested_interaction = 1;
    return 1; //? what? why is this non void?
}

Uint8 npc_is_interacting(){
    return (interaction_manager.interacting != NULL);
}

void draw_shop_ui(){
    if(interaction_manager.interacting){
        NpcProps* props = (NpcProps*)interaction_manager.interacting->data;
        
        SDL_Rect r = {0, 600, 1200, 120};
        SDL_SetRenderDrawColor(gf2d_graphics_get_renderer(), 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderFillRect(gf2d_graphics_get_renderer(), &r);
        if(props->textbox_text) gf2d_sprite_draw_image(props->textbox_text, vector2d(16, 616));
        
        if(props->is_shop){
            Vector2D draw_item_pos = interaction_manager.interacting->hurtbox.pos;
            vector2d_sub(draw_item_pos, draw_item_pos, get_camera_pos());
            draw_item_pos.x += 16;
            draw_item_pos.y -= 16;
            Vector2D scale = vector2d(0.25,0.25);
            gf2d_sprite_draw(inventory_manager_consumables_img(), draw_item_pos, &scale, NULL, NULL, NULL, NULL, props->sell_type);
        }
    }
}