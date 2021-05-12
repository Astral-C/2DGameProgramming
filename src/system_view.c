#include <simple_logger.h>

#include "gf2d_mouse.h"
#include "gf2d_draw.h"

#include "system_view.h"

typedef struct
{
    System *system;
}SystemWindowData;


int system_view_draw(Window *win)
{
    SystemWindowData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = (SystemWindowData*)win->data;
    system_draw_system_view(data->system,vector2d(130  ,130));
    return 1;
}

int system_view_free(Window *win)
{
    SystemWindowData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = (SystemWindowData*)win->data;
    free(data);
    return 0;
}

int system_view_update(Window *win,List *updateList)
{
    SystemWindowData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = (SystemWindowData*)win->data;
    return 0;
}

Window *system_view_window(System *system)
{
    Window *win;
    SystemWindowData *data;
    win = gf2d_window_load("menus/system_view.json");
    if (!win)
    {
        slog("failed to load system view window");
        return NULL;
    }
    win->no_draw_generic = 1;
    win->draw = system_view_draw;
    win->update = system_view_update;
    win->free_data = system_view_free;
    data = gfc_allocate_array(sizeof(SystemWindowData),1);
    data->system = system;
    win->data = data;
    return win;

}


/*file's end*/