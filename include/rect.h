#ifndef __RECT_H__
#define __RECT_H__
#include "gfc_vector.h"

#define COL_RIGHT  1 << 4
#define COL_LEFT   1 << 3
#define COL_CEIL    1 << 2
#define COL_FLOOR 1
#define COLMASK_NONE    0

typedef struct RECT_S {
    Vector2D pos;
    Vector2D size;
} Rect;

#define rect_collidep(p, r) (p.x > r.pos.x) && (p.y > r.pos.y) && (p.y < (r.pos.y + r.size.y)) && (p.x < (r.pos.x + r.size.x))
#define rect_collider(r1,r2) (r1.pos.x + r1.size.x > r2.pos.x) \
                          && (r1.pos.x < r2.pos.x + r2.size.x) \
                          && (r1.pos.y + r1.size.y > r2.pos.y) \
                          && (r1.pos.y < r2.pos.y + r2.size.y)
#endif