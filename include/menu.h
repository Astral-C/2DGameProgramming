#ifndef __MENU_SYS_H__
#define __MENU__SYS_H__
#include <SDL.h>

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