#pragma once
// Publish the configuration export and shared text-override services once at startup.
namespace config_services { void Initialize(); }
namespace text_services { void Initialize(); }
