#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_H

// The parts of the Xtransceiver's calls with the game's characters (overlay 259), one per file of the ROM's
// "local_tvt_*.c". Every name here is ours

#include "types.h"
#include "communication/xtransceiver/local_tvt_calldata.h"
#include "communication/xtransceiver/local_tvt_callmode.h"
#include "communication/xtransceiver/local_tvt_callselect.h"
#include "communication/xtransceiver/local_tvt_chara.h"
#include "communication/xtransceiver/local_tvt_common.h"
#include "communication/xtransceiver/local_tvt_graphic.h"
#include "communication/xtransceiver/local_tvt_message.h"
#include "communication/xtransceiver/local_tvt_person.h"
#include "communication/xtransceiver/local_tvt_person_chelen.h"
#include "communication/xtransceiver/local_tvt_person_doctor.h"
#include "communication/xtransceiver/local_tvt_person_mother.h"
#include "communication/xtransceiver/local_tvt_person_rival.h"
#include "communication/xtransceiver/local_tvt_person_secret.h"
#include "communication/xtransceiver/local_tvt_person_support.h"
#include "communication/xtransceiver/local_tvt_scriptmode.h"
#include "communication/xtransceiver/local_tvt_util.h"
#include "communication/xtransceiver/xtransceiver.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/savedata/save_control.h"

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_H
