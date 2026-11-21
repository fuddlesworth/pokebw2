#ifndef POKEBW2_APP_WFC_USER_INFO_WARNING_H
#define POKEBW2_APP_WFC_USER_INFO_WARNING_H

#include "types.h"
#include "gfl/overlay.h"

// The warning that the Nintendo Wi-Fi Connection user information may have been erased (wfc_user_info_warning.c)
#define OVERLAY_WFC_USER_INFO_WARNING OVERLAY_ID(32)

// Shows the warning on its own screen, and returns when A or B is pressed
void WFCUserInfoWarning_Show(void);

#endif // POKEBW2_APP_WFC_USER_INFO_WARNING_H
