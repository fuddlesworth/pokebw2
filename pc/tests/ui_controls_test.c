#include "check.h"
#include "ui_layout.h"
#include <stdio.h>

int main(void) {
    PcUiControls controls = { 0 };
    int owner = 0, nextOwner = 0;
    PcUiControls_Begin(&controls, &owner, 1);
    PcUiShape tab = { .rect = { 26, 101, 305, 43 }, .topRight = 40 };
    CHECK(PcUiControls_Add(&controls, tab, (PcUiAction){ PC_UI_ACTION_TAB, 2 }));
    CHECK(PcUiControls_Hit(&controls, &owner, 1, 180, 120).value == 2);
    CHECK(PcUiControls_Hit(&controls, &owner, 1, 329, 102).kind == PC_UI_ACTION_NONE);
    CHECK(PcUiControls_Hit(&controls, &owner, 1, 329, 142).kind == PC_UI_ACTION_TAB);
    CHECK(PcUiControls_Hit(&controls, &owner, 2, 180, 120).kind == PC_UI_ACTION_NONE);
    CHECK(PcUiControls_Hit(&controls, &nextOwner, 1, 180, 120).kind == PC_UI_ACTION_NONE);
    CHECK(!PcUiControls_Add(&controls, tab, (PcUiAction){ PC_UI_ACTION_NONE, 0 }));
    PcUiRect first = { 42, 213, 608, 58 };
    for (unsigned row = 0; row < 6; row++) {
        PcUiShape shape = { .rect = PcUi_RowRect(first, 65, row), .topRight = 16 };
        CHECK(PcUiControls_Add(&controls, shape, (PcUiAction){ PC_UI_ACTION_ROW, (int)row }));
        CHECK(PcUiControls_Hit(&controls, &owner, 1, 100, shape.rect.y + 57.5f).value == (int)row);
        CHECK(PcUiControls_Hit(&controls, &owner, 1, 100, shape.rect.y + 60).kind == PC_UI_ACTION_NONE);
    }
    PcUiShape sloped = { .rect = { 958, 102, 306, 40 }, .topRight = 40, .bottomLeft = 40 };
    CHECK(!PcUi_Contains(sloped, 960, 140));
    CHECK(PcUi_Contains(sloped, 960, 103));
    CHECK(!PcUi_Contains(sloped, 1262, 103));
    CHECK(PcUi_Contains(sloped, 1262, 140));
    CHECK(!PcUi_Contains(sloped, 1000, 142));
    PcUiControls_Begin(&controls, &nextOwner, 0);
    CHECK(PcUiControls_Hit(&controls, &owner, 1, 180, 120).kind == PC_UI_ACTION_NONE);
    CHECK(PcUiControls_Add(&controls, tab, (PcUiAction){ PC_UI_ACTION_TAB, 1 }));
    CHECK(PcUiControls_Add(&controls, tab, (PcUiAction){ PC_UI_ACTION_MENU, 3 }));
    CHECK(PcUiControls_Hit(&controls, &nextOwner, 0, 180, 120).kind == PC_UI_ACTION_MENU);
    puts("UI controls: bevels, gaps, scope transitions and overlay precedence pass");
    return 0;
}
