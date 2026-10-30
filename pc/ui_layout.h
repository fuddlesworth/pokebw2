#ifndef POKEBW2_PC_UI_LAYOUT_H
#define POKEBW2_PC_UI_LAYOUT_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    float x, y, width, height;
} PcUiRect;
/* Corner cuts are measured along both axes, so bevels retain a 45 degree slope. */
typedef struct {
    PcUiRect rect;
    float topLeft, topRight, bottomRight, bottomLeft;
} PcUiShape;
typedef enum {
    PC_UI_ACTION_NONE,
    PC_UI_ACTION_KEY,
    PC_UI_ACTION_TAB,
    PC_UI_ACTION_ROW,
    PC_UI_ACTION_MENU,
    PC_UI_ACTION_MARK,
    PC_UI_ACTION_CUSTOM
} PcUiActionKind;
typedef struct {
    PcUiActionKind kind;
    int value;
} PcUiAction;
typedef struct {
    PcUiShape shape;
    PcUiAction action;
} PcUiControl;
typedef struct {
    const void *owner;
    unsigned phase;
    size_t count;
    PcUiControl controls[128];
} PcUiControls;

PcUiRect PcUi_RowRect(PcUiRect first, float stride, unsigned row);
bool PcUi_Contains(PcUiShape shape, float x, float y);
void PcUiControls_Begin(PcUiControls *controls, const void *owner, unsigned phase);
bool PcUiControls_Add(PcUiControls *controls, PcUiShape shape, PcUiAction action);
PcUiAction PcUiControls_Hit(const PcUiControls *controls, const void *owner, unsigned phase, float x, float y);

#endif
