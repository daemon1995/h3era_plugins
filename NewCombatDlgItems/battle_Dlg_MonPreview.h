#pragma once

#include "header.h"

#define o_BPP ByteAt(0x5FA228 + 3) << 3
#define DLG_HEIGHT_ADD 86


void Dlg_MonPreview_HooksInit(PatcherInstance* pi);
