#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#define NLOHMAN_JSON
#define MAIN_MENU_API
// add headers that you want to pre-compile here
#include "..\headers\header.h"
#include "..\headers\era\eraModList.hpp"
#include "AssemblyInformation.h"
#include "NotificationPanel.h"
#include "UserNotification.h"

namespace panelText
{
constexpr LPCSTR HIDE_ONE = "era.notifications.panel.hide_one";
constexpr LPCSTR HIDE_ALL = "era.notifications.panel.hide_all";
constexpr LPCSTR TITLE = "era.notifications.panel.title";
constexpr LPCSTR BUTTON_FORMAT = "era.notifications.panel.button";

constexpr LPCSTR LAUNCHER_PATH = "era.notifications.panel.launcher_path";
constexpr LPCSTR OPEN_URL = "era.notifications.user.open_web_page";
constexpr LPCSTR OPEN_FILE = "era.notifications.user.open_external_file";

} // namespace panelText
