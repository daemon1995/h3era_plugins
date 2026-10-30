#pragma once

#include <stdint.h>

namespace EventNotificationsApi
{
// GetProcAddress(GetModuleHandleA(MODULE), ADD_EXPORT).
// Call on the game thread during a map session. Both strings are copied.
// Returns 1 if queued, 0 on invalid input, wrong thread or allocation failure.
static const char MODULE[] = "Interface_EventNotifications.era";
static const char ADD_EXPORT[] = "AddEventNotification";
typedef int32_t(__stdcall *AddProc)(const char *displayedText, const char *clickedText);
} // namespace EventNotificationsApi
