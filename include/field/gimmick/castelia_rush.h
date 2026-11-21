#ifndef POKEBW2_FIELD_GIMMICK_CASTELIA_RUSH_H
#define POKEBW2_FIELD_GIMMICK_CASTELIA_RUSH_H

#include "types.h"
#include "field/field_async_proc.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// Overlay 121: the crowds of Castelia City's streets, a field process. Its function and data names are our own;
// overlay 36's field calls CasteliaRush_ClearBalloons

#define OVERLAY_CASTELIA_RUSH OVERLAY_ID(121)

extern const FieldAsyncProcDef CASTELIA_RUSH_PROC_DEF;

// Removes the passers-by's speech balloons, as the field does before it shows a message
void CasteliaRush_ClearBalloons(CasteliaRush *rush);

#endif // POKEBW2_FIELD_CASTELIA_RUSH_H
