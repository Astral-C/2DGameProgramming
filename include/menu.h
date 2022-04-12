#ifndef __MENU_SYS_H__
#define __MENU__SYS_H__
#include <SDL.h>
#include "pg_entity.h"
#include "map.h"
#include "npc.h"
#include "quest.h"

/*
 *      Menu System, uses Nuklear as a base
 *      https://github.com/Immediate-Mode-UI/Nuklear
 */

void init_menu();
void menu_update();
struct nk_context* menu_get_nk_context();
void menu_draw();

int game_paused();

#endif