#include "../settings.h"
#include "check.h"
#include "gfl/backup_card.h"
#include "gfl/key.h"
#include "nitro/host_card.h"
#include "save/config.h"
#include <stdio.h>
int main(void) {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    CHECK(SDL_Init(SDL_INIT_GAMEPAD));
    char path[256];
    SDL_snprintf(path, sizeof(path), "settings-test-%llu.sav", (unsigned long long)SDL_GetPerformanceCounter());
    CHECK(PcCard_OpenBackup(path));
    PcSettings_Load();
    CHECK(PcSettings_Get()->volume == 10 && PcSettings_Get()->keys[4] == SDL_SCANCODE_Z);
    PcSettings draft = *PcSettings_Get();
    draft.volume = 3;
    draft.frameLimit = 3;
    draft.gameValid = 1;
    draft.textSpeed = 2;
    draft.alternate[4] = SDL_SCANCODE_F12;
    draft.integerScale = 1;
    draft.antialiasing = 2;
    draft.invertWheel = 1;
    draft.spriteFilter = 1;
    CHECK(PcSettings_Save(&draft));
    PcCard_CloseBackup();
    CHECK(PcCard_OpenBackup(path));
    PcSettings_Load();
    CHECK(PcSettings_Get()->volume == 3 && PcSettings_Get()->frameLimit == 3 && PcSettings_Get()->textSpeed == 2);
    CHECK(PcSettings_Get()->alternate[4] == SDL_SCANCODE_F12 && PcSettings_Get()->antialiasing == 2);
    CHECK(PcSettings_Get()->invertWheel && PcSettings_Get()->spriteFilter);
    CHECK(PcSettings_Key(SDL_SCANCODE_F12) == PAD_BUTTON_A);
    PcViewport viewport = PcSettings_Viewport(1280, 720);
    CHECK(viewport.x == 64 && viewport.y == 36 && viewport.width == 1152 && viewport.height == 648);
    float x, y;
    CHECK(PcViewport_MapPointer(viewport, 640, 360, &x, &y) && x == 192 && y == 108);
    CHECK(!PcViewport_MapPointer(viewport, 63, 360, &x, &y));
    viewport = PcSettings_Viewport(320, 180);
    CHECK(viewport.width == 320 && viewport.height == 180);
    Config config = { 0 };
    PcSettings_ApplyGame(&config);
    CHECK(Config_GetTextSpeed(&config) == 2);
    draft.volume = 6;
    CHECK(PcSettings_Save(&draft));
    PcCard_CloseBackup();
    SDL_IOStream *file = SDL_IOFromFile(path, "r+b");
    CHECK(file);
    unsigned char data[4096];
    for (unsigned i = 0; i < 128; i++) {
        CHECK(SDL_ReadIO(file, data, sizeof(data)) == sizeof(data));
        for (unsigned j = 0; j < sizeof(data); j++)
            CHECK(data[j] == 255);
    }
    // Damage the newest record: reopening must recover the preceding committed settings.
    CHECK(SDL_SeekIO(file, 512 * 1024 + 272 + 20, SDL_IO_SEEK_SET) >= 0);
    CHECK(SDL_WriteIO(file, "bad", 3) == 3 && SDL_CloseIO(file));
    CHECK(PcCard_OpenBackup(path));
    PcSettings_Load();
    CHECK(PcSettings_Get()->volume == 3);
    // Version two retains its display/input settings and gains standard controller bindings.
    Uint8 versionTwo[256];
    CHECK(PcCard_ReadSettings(versionTwo, sizeof(versionTwo)));
    versionTwo[0] = 2;
    SDL_memset(versionTwo + 72, 255, 46);
    CHECK(PcCard_WriteSettings(versionTwo, sizeof(versionTwo)));
    PcSettings_Load();
    CHECK(PcSettings_Get()->controllerButtons[5] == SDL_GAMEPAD_BUTTON_EAST);
    CHECK(PcSettings_Get()->leftStick && !PcSettings_Get()->controllerGuid[0]);
    CHECK(PcSettings_Get()->antialiasing == 2 && PcSettings_Get()->alternate[4] == SDL_SCANCODE_F12);
    draft.keys[0] = draft.keys[1];
    CHECK(!PcSettings_Save(&draft));
    CHECK(PcSettings_Get()->volume == 3);
    unsigned char marker = 0x5a, restored = 0;
    CHECK(writeFlashWaitSuccess(8, &marker, 1, NULL));
    CHECK(readFlashWaitSuccess(8, &restored, 1, NULL) && restored == marker);
    PcSettings_Load();
    CHECK(PcSettings_Get()->volume == 3);
    // A version-one payload keeps its old bindings and receives safe defaults for new controls.
    Uint8 legacy[256] = { 0 };
    CHECK(PcCard_ReadSettings(legacy, sizeof(legacy)));
    legacy[0] = 1;
    CHECK(PcCard_WriteSettings(legacy, sizeof(legacy)));
    PcSettings_Load();
    CHECK(PcSettings_Get()->controller && PcSettings_Get()->deadzone == 2);
    CHECK(PcSettings_Get()->alternate[4] == 0 && PcSettings_Get()->antialiasing == 0);
    // Version four preserves a selected device identity; older records ignore its bytes.
    draft = *PcSettings_Get();
    SDL_memcpy(draft.controllerGuid, "00112233445566778899aabbccddeeff", 33);
    draft.controllerSlot = 2;
    draft.controllerPathHash = UINT64_C(0x1122334455667788);
    CHECK(PcSettings_Save(&draft));
    PcSettings_Load();
    CHECK(PcSettings_Get()->controllerSlot == 2);
    CHECK(PcSettings_Get()->controllerPathHash == UINT64_C(0x1122334455667788));
    Uint8 versionThree[256];
    CHECK(PcCard_ReadSettings(versionThree, sizeof(versionThree)));
    versionThree[0] = 3;
    SDL_memset(versionThree + 118, 255, 9);
    CHECK(PcCard_WriteSettings(versionThree, sizeof(versionThree)));
    PcSettings_Load();
    CHECK(!PcSettings_Get()->controllerSlot && !PcSettings_Get()->controllerPathHash);
    SDL_VirtualJoystickDesc description = { 0 };
    SDL_INIT_INTERFACE(&description);
    description.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    description.naxes = SDL_GAMEPAD_AXIS_COUNT;
    description.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    description.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
    description.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
    description.name = "Settings regression controller";
    SDL_JoystickID id = SDL_AttachVirtualJoystick(&description);
    CHECK(id && SDL_IsGamepad(id));
    SDL_Joystick *joystick = SDL_OpenJoystick(id);
    CHECK(joystick);
    draft = *PcSettings_Get();
    SDL_GUIDToString(SDL_GetGamepadGUIDForID(id), draft.controllerGuid, sizeof(draft.controllerGuid));
    CHECK(PcSettings_Save(&draft));
    PcSettings_Load();
    CHECK(!SDL_strcmp(PcSettings_Get()->controllerGuid, draft.controllerGuid));
    CHECK(!SDL_strcmp(PcSettings_ControllerName(&draft), description.name));
    SDL_Event added = { .gdevice = { .type = SDL_EVENT_GAMEPAD_ADDED } };
    added.gdevice.which = id;
    PcSettings_InputEvent(&added);
    CHECK(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, true));
    SDL_UpdateJoysticks();
    CHECK(PcSettings_Gamepad() & PAD_BUTTON_A);
    CHECK(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH, false));
    CHECK(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_EAST, true));
    SDL_UpdateJoysticks();
    CHECK(PcSettings_Gamepad() == PAD_BUTTON_B);
    CHECK(PcSettings_MenuGamepad() == PAD_BUTTON_B);
    draft.controllerButtons[4] = SDL_GAMEPAD_BUTTON_EAST;
    draft.controllerButtons[5] = SDL_GAMEPAD_BUTTON_SOUTH;
    CHECK(PcSettings_Save(&draft));
    PcSettings_Load();
    CHECK(PcSettings_Gamepad() == PAD_BUTTON_A);
    CHECK(PcSettings_MenuGamepad() == PAD_BUTTON_B);
    CHECK(SDL_SetJoystickVirtualButton(joystick, SDL_GAMEPAD_BUTTON_EAST, false));
    CHECK(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 5000));
    SDL_UpdateJoysticks();
    CHECK(!(PcSettings_Gamepad() & PAD_KEY_RIGHT));
    CHECK(SDL_SetJoystickVirtualAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX, 20000));
    SDL_UpdateJoysticks();
    CHECK(PcSettings_Gamepad() & PAD_KEY_RIGHT);
    draft.leftStick = 0;
    CHECK(PcSettings_Save(&draft));
    CHECK(!(PcSettings_Gamepad() & PAD_KEY_RIGHT));
    CHECK(PcSettings_MenuGamepad() & PAD_KEY_RIGHT);
    draft.controller = 0;
    CHECK(PcSettings_Save(&draft));
    CHECK(!PcSettings_Gamepad() && !PcSettings_MenuGamepad());
    draft.controller = 1;
    CHECK(PcSettings_Save(&draft));
    draft.controllerButtons[0] = draft.controllerButtons[1];
    CHECK(!PcSettings_Save(&draft));
    draft = *PcSettings_Get();
    SDL_memset(draft.controllerGuid, 'x', sizeof(draft.controllerGuid));
    CHECK(!PcSettings_Save(&draft));
    SDL_JoystickID second = SDL_AttachVirtualJoystick(&description);
    CHECK(second && second != id && SDL_IsGamepad(second));
    char firstGuid[33], secondGuid[33];
    SDL_GUIDToString(SDL_GetGamepadGUIDForID(id), firstGuid, sizeof(firstGuid));
    SDL_GUIDToString(SDL_GetGamepadGUIDForID(second), secondGuid, sizeof(secondGuid));
    CHECK(!SDL_strcmp(firstGuid, secondGuid));
    draft = *PcSettings_Get();
    SDL_zero(draft.controllerGuid);
    draft.controllerSlot = 0;
    draft.controllerPathHash = 0;
    int count = 0, target = -1;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    for (int i = 0; i < count; i++)
        if (ids[i] == second)
            target = i;
    SDL_free(ids);
    CHECK(target >= 0);
    for (int i = 0; i <= target; i++)
        PcSettings_CycleController(&draft, 1);
    CHECK(!SDL_strcmp(draft.controllerGuid, secondGuid));
    CHECK(SDL_strstr(PcSettings_ControllerName(&draft), " #"));
    CHECK(PcSettings_Save(&draft));
    PcSettings_Load();
    CHECK(PcSettings_Controller() && SDL_GetGamepadID(PcSettings_Controller()) == second);
    draft.controllerPathHash = UINT64_MAX;
    CHECK(PcSettings_Save(&draft));
    PcSettings_Load();
    CHECK(PcSettings_Controller() && SDL_GetGamepadID(PcSettings_Controller()) == second);
    CHECK(SDL_DetachVirtualJoystick(second));
    CHECK(!PcSettings_Controller());
    CHECK(!SDL_strcmp(PcSettings_ControllerName(PcSettings_Get()), "Disconnected"));
    PcSettings_CloseInput();
    SDL_CloseJoystick(joystick);
    CHECK(SDL_DetachVirtualJoystick(id));
    CHECK(!PcSettings_Controller());
    CHECK(!SDL_strcmp(PcSettings_ControllerName(PcSettings_Get()), "Disconnected"));
    PcCard_CloseBackup();
    CHECK(SDL_RemovePath(path));
    SDL_Quit();
    return 0;
}
