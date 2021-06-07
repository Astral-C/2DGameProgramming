#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"
#include "gfc_input.h"
#include "gfc_callbacks.h"

#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_list.h"
#include "gf2d_element_label.h"
#include "gf2d_element_actor.h"
#include "gf2d_element_button.h"
#include "gf2d_draw.h"
#include "gf2d_shape.h"
#include "gf2d_mouse.h"
#include "gf2d_font.h"

#include "camera.h"
#include "windows_common.h"
#include "message_buffer.h"
#include "buy_menu.h"

extern void exitGame();
extern void exitCheck();


typedef struct
{
    int *choice;
    int selection;
    TextLine filename;
    Callback *onBuy;
    Callback *onCancel;
    SJson *json;
    SJson *items;
}BuyMenuData;

int buy_menu_free(Window *win)
{
    BuyMenuData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = win->data;
    
    gfc_callback_free(data->onBuy);
    gfc_callback_free(data->onCancel);
    
    free(data);
    return 0;
}

int buy_menu_draw(Window *win)
{
    return 0;
}

void buy_menu_select(Window *win,int index)
{
    SJson *item;
    TextLine textline;
    BuyMenuData* data;

    if (!win)return;

    data = (BuyMenuData*)win->data;
    if (!data)return;

    data->selection = index;
    item = sj_array_get_nth(data->items,index);
    if (!item)return;
    
    gf2d_element_label_set_text(gf2d_window_get_element_by_id(win,101),(char *)sj_get_string_value(sj_object_get_value(item,"name")));
    
}

int buy_menu_update(Window *win,List *updateList)
{
    int i,count,n;
    Element *e;
    BuyMenuData* data;
    if (!win)return 0;
    if (!updateList)return 0;
    data = (BuyMenuData*)win->data;
    if (!data)return 0;
    if (gfc_input_command_released("cancel"))
    {
        gf2d_window_free(win);
        return 1;
    }

    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        switch(e->index)
        {
            case 151:
                if (data->choice)
                {
                    *data->choice = data->selection;
                }
                if (data->onBuy)
                {
                    gfc_callback_call(data->onBuy);
                }
                gf2d_window_free(win);
                return 1;
            case 152:
                if (data->onCancel)
                {
                    gfc_callback_call(data->onCancel);
                }
                gf2d_window_free(win);
                return 1;
        }
        if ((e->index >= 301)&& (e->index < 400))
        {
            n = e->index - 301;
            buy_menu_select(win,n);
            return 1;
        }

    }
    return gf2d_window_mouse_in(win);
}


Window *buy_menu(Empire *empire,const char *shopConfig,int *choice, void(*onBuy)(void *),void(*onCancel)(void *),void *cData)
{
    ButtonElement *button;
    LabelElement *label;
    ActorElement *actor;
    Element *e,*p,*l,*a;
    
    SJson *json,*items,*item;
    int i,c;
    
    Window *win;
    BuyMenuData* data;
    
    win = gf2d_window_load("menus/buy_menu.json");
    if (!win)
    {
        slog("failed to load buy menu");
        return NULL;
    }
    json = sj_load(shopConfig);
    if (!json)
    {
        slog("failed to load shop config data");
        gf2d_window_free(win);
        return NULL;
    }
    win->update = buy_menu_update;
    win->free_data = buy_menu_free;
    win->draw = buy_menu_draw;
    data = (BuyMenuData*)gfc_allocate_array(sizeof(BuyMenuData),1);
    data->onBuy = gfc_callback_new(onBuy,cData);
    data->choice = choice;
    data->onCancel = gfc_callback_new(onCancel,cData);
    data->json = json;
    // parse out the json
    p = gf2d_window_get_element_by_id(win,300);

    items = sj_object_get_value(json,"items");
    if (items)
    {
        c = sj_array_get_count(items);
        for (i = 0; i < c; i++)
        {
            item = sj_array_get_nth(items,i);
            if (!item)continue;
            e = gf2d_element_new_full(
                p,
                301 + i,
                "button",
                gf2d_rect(0,0,300,37),
                gfc_color8(255,255,255,255),
                0,
                gfc_color8(255,255,255,255),
                0);
            l = gf2d_element_new_full(
                e,
                3000 + i,
                "label",
                gf2d_rect(0,0,300,37),
                gfc_color8(255,255,255,255),
                0,
                gfc_color8(255,255,255,255),
                0);
            a = gf2d_element_new_full(
                e,
                30000 + i,
                "actor",
                gf2d_rect(0,0,300,37),
                gfc_color8(255,255,255,255),
                0,
                gfc_color8(255,255,255,255),
                0);
            
            label = gf2d_element_label_new_full(
                (char *)sj_get_string_value(sj_object_get_value(item,"name")),
                gfc_color8(255,255,255,255),
                FT_H5,
                LJ_Center,
                LA_Middle,
                0);
            gf2d_element_make_label(l,label);

            actor = gf2d_element_actor_new_full("actors/list_button.actor", "idle",vector2d(1,1),NULL);
            gf2d_element_make_actor(a,actor);

            
            button = gf2d_element_button_new_full(
                l,
                a,
                gfc_color8(255,255,255,255),
                gfc_color8(255,255,255,255),
                0);
            gf2d_element_make_button(e,button);
            
            gf2d_element_list_add_item(p,e);
        }
    }
    data->items = items;
    win->data = data;
    
    buy_menu_select(win,0);
    return win;
}


/*eol@eof*/